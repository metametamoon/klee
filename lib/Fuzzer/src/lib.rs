#![allow(non_snake_case)]

use core::slice;
use std::{
    ffi::c_int,
    marker::PhantomData,
    time::{Instant, Duration},
    fmt::Debug
};

use libafl::{
    Error,
    bolts::{rands::{Rand, StdRand},
            shmem::{StdShMemProvider, ShMemProvider},
            tuples::{tuple_list, tuple_list_type, Named},
            AsSlice, AsMutSlice, AsIter},
    corpus::{Corpus, InMemoryCorpus},
    events::SimpleEventManager,
    executors::{ExitKind, InProcessForkExecutor},
    feedbacks::{MaxMapFeedback, Feedback},
    fuzzer::{Fuzzer, StdFuzzer},
    generators::Generator,
    inputs::{BytesInput, HasTargetBytes, HasBytesVec, Input},
    monitors::NopMonitor,
    mutators::{StdScheduledMutator, BitFlipMutator, ByteFlipMutator,
               ByteIncMutator, ByteDecMutator, ByteNegMutator, ByteRandMutator,
               ByteAddMutator, WordAddMutator, DwordAddMutator,
               QwordAddMutator, ByteInterestingMutator, WordInterestingMutator,
               DwordInterestingMutator},
    observers::{StdMapObserver, MapObserver},
    schedulers::{QueueScheduler, CoverageAccountingScheduler, IndexesLenTimeMinimizerScheduler, MinimizerScheduler},
    stages::StdMutationalStage,
    state::{StdState, HasRand, HasNamedMetadata, HasClientPerfMonitor,
            HasSolutions}, prelude::{HitcountsMapObserver, havoc_mutations, current_nanos},
};

use libafl_targets::{EDGES_MAP_PTR, MAX_EDGES_NUM};

#[repr(C)]
pub struct FuzzInfo {
    pub harness: extern "C" fn(*const u8, usize) -> c_int,
    pub map_size: usize,
    pub timeout: u64,
    pub data_size: usize,
    pub seed: *mut u8,
    pub solution: *mut u8
}

#[derive(Clone, Debug)]
pub struct SeedGenerator<S>
where
    S: HasRand
{
    seed: *mut u8,
    seed_size: usize,
    phantom: PhantomData<S>
}

impl<S> Generator<BytesInput, S> for SeedGenerator<S>
where
    S: HasRand
{
    fn generate(&mut self, state: &mut S) -> Result<BytesInput, Error> {
        let bytes;
        unsafe {
            bytes = slice::from_raw_parts(self.seed, self.seed_size).to_vec();
        };
        Ok(BytesInput::new(bytes))
    }

    fn generate_dummy(&self, _state: &mut S) -> BytesInput {
        BytesInput::new(vec![0; self.seed_size])
    }
}

impl<S> SeedGenerator<S>
where
    S: HasRand,
{
    #[must_use]
    pub fn new(seed: *mut u8, seed_size: usize) -> Self {
        Self {
            seed,
            seed_size,
            phantom: PhantomData,
        }
    }
}

#[derive(Clone, Debug)]
pub struct RandBytesFixedGenerator<S>
where
    S: HasRand,
{
    size: usize,
    phantom: PhantomData<S>,
}

impl<S> Generator<BytesInput, S> for RandBytesFixedGenerator<S>
where
    S: HasRand,
{
    fn generate(&mut self, state: &mut S) -> Result<BytesInput, Error> {
        let random_bytes: Vec<u8> = (0..self.size)
            .map(|_| state.rand_mut().below(256) as u8)
            .collect();
        Ok(BytesInput::new(random_bytes))
    }

    fn generate_dummy(&self, _state: &mut S) -> BytesInput {
        BytesInput::new(vec![0; self.size])
    }
}

impl<S> RandBytesFixedGenerator<S>
where
    S: HasRand,
{
    #[must_use]
    pub fn new(size: usize) -> Self {
        Self {
            size,
            phantom: PhantomData,
        }
    }
}

pub type MyHavocMutationsType = tuple_list_type!(
    ByteFlipMutator,
    ByteIncMutator,
    ByteDecMutator,
    ByteNegMutator,
    ByteRandMutator,
    ByteAddMutator,
    WordAddMutator,
    DwordAddMutator,
    QwordAddMutator,
    ByteInterestingMutator,
    WordInterestingMutator,
    DwordInterestingMutator,
    BitFlipMutator,
);

pub fn my_havoc_mutations() -> MyHavocMutationsType {
    tuple_list!(
        ByteFlipMutator::new(),
        ByteIncMutator::new(),
        ByteDecMutator::new(),
        ByteNegMutator::new(),
        ByteRandMutator::new(),
        ByteAddMutator::new(),
        WordAddMutator::new(),
        DwordAddMutator::new(),
        QwordAddMutator::new(),
        ByteInterestingMutator::new(),
        WordInterestingMutator::new(),
        DwordInterestingMutator::new(),
        BitFlipMutator::new(),
    )
}

#[derive(Clone, Debug)]
pub struct SATFeedback<O>
where
    O: MapObserver<Entry = u8>,
    for<'it> O: AsIter<'it, Item = u8>,
{
    sat_index : usize,
    name : String,
    observer_name: String,
    phantom: PhantomData<O>
}

impl <O> Named for SATFeedback<O>
where
    O: MapObserver<Entry = u8>,
    for<'it> O: AsIter<'it, Item = u8>,
{
    #[inline]
    fn name(&self) -> &str {
        self.name.as_str()
    }
}

impl<O,I,S> Feedback<I,S> for SATFeedback<O>
where
    O: MapObserver<Entry = u8>,
    for<'it> O: AsIter<'it, Item = u8>,
    I: Input,
    S: HasNamedMetadata + HasClientPerfMonitor + Debug,
{
    fn is_interesting<EM, OT>(
        &mut self,
        _state: &mut S,
        _manager: &mut EM,
        _input: &I,
        observers: &OT,
        _exit_kind: &ExitKind,
    ) -> Result<bool, Error>
    where
        EM: libafl::prelude::EventFirer<I>,
        OT: libafl::prelude::ObserversTuple<I, S> {
        let observer = observers.match_name::<O>(&self.observer_name).unwrap();
        if *observer.get(self.sat_index) == 1 {
            Ok(true)
        } else {
            Ok(false)
        }
    }
}

impl<O> SATFeedback<O>
where
    O: MapObserver<Entry = u8>,
    for<'it> O: AsIter<'it, Item = u8>,
{
    #[must_use]
    pub fn new(map_observer: &O, sat_index : usize) -> Self {
        Self {
            name: "SATFeedback".to_string() + map_observer.name(),
            sat_index,
            observer_name: map_observer.name().to_string(),
            phantom: PhantomData,
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn __record_coverage(index: usize) {
    (EDGES_MAP_PTR as *mut u8).add(index).write(1);
}

#[no_mangle]
pub unsafe extern "C" fn fuzz_internal(fi : FuzzInfo) -> bool {

    MAX_EDGES_NUM = fi.map_size;

    let mut shmem_provider = StdShMemProvider::new().unwrap();
    let mut shmem = shmem_provider.new_shmem(fi.map_size).unwrap();
    let shmem_buf = shmem.as_mut_slice();
    EDGES_MAP_PTR = shmem_buf.as_mut_ptr();

    let observer = StdMapObserver::new_from_ptr("edges", EDGES_MAP_PTR,
                                                fi.map_size);
    let observer = HitcountsMapObserver::new(observer);

    let mut feedback = MaxMapFeedback::new(&observer);
    let mut objective = SATFeedback::new(&observer, fi.map_size - 1);

    let mut state = StdState::new(
        StdRand::with_seed(0),
        InMemoryCorpus::new(),
        InMemoryCorpus::new(),
        &mut feedback,
        &mut objective
    ).unwrap();

    let scheduler = QueueScheduler::new();

    let mut fuzzer = StdFuzzer::new(scheduler, feedback, objective);

    let mut mgr = SimpleEventManager::new(NopMonitor::new());

    let mut harness = |input: &BytesInput| {
        let target = input.target_bytes();
        let buf = target.as_slice();
        (fi.harness)(buf.as_ptr(), buf.len());
        ExitKind::Ok
    };

    let mut executor = InProcessForkExecutor::new(
        &mut harness,
        tuple_list!(observer),
        &mut fuzzer,
        &mut state,
        &mut mgr,
        shmem_provider
    ).expect("Failed to create the Executor");

    let mut generator = SeedGenerator::new(fi.seed, fi.data_size);
    state.generate_initial_inputs_forced(&mut fuzzer, &mut executor,
                                         &mut generator, &mut mgr, 1)
        .expect("Failed to generate");

    // let mut generator = RandBytesFixedGenerator::new(fi.data_size);
    // state.generate_initial_inputs_forced(&mut fuzzer, &mut executor,
    //                                      &mut generator, &mut mgr, 1)
    //     .expect("Failed to generate");

    let mutator = StdScheduledMutator::new(my_havoc_mutations());
    let mut stages = tuple_list!(StdMutationalStage::new(mutator));

    let fuzzing_started = Instant::now();
    let timeout = Duration::from_micros(fi.timeout);

    let res = loop {
        fuzzer.fuzz_one(&mut stages, &mut executor,
                        &mut state, &mut mgr).unwrap();
        if state.solutions().count() > 0 {
            let solution = state.solutions().get(0).unwrap();
            break Some(solution);
        }
        if fuzzing_started.elapsed() > timeout {
            break None;
        }
    };

    match res {
        Some(solution) => {
            let bytes_iter = solution.borrow().clone();
            let bytes_iter =
                bytes_iter.input().as_ref().unwrap().bytes().iter();
            for (pos, byte) in bytes_iter.enumerate() {
                fi.solution.add(pos).write(*byte);
            }
            println!("Fuzzing successful, time to fuzz {:?} ms",
                     fuzzing_started.elapsed().as_millis());
            return true
        },
        None => {
            println!("Fuzzing failed");
            return false
        }
    }
}
