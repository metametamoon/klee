#!/bin/python3

import json
import re
from argparse import ArgumentParser

Cooddy_name_pattern = re.compile(r"^(?P<name>.*)\((?P<mangled_name>[^()]*)\)$")


def getNames(name: str):
    m = Cooddy_name_pattern.fullmatch(name)
    if m:
        return m.group("name"), m.group("mangled_name")
    return name, name


def transform_annotation_with_taint_statements(annotation, funcName):
    transformed_annotation = []
    for i, annotation_for_param in enumerate(annotation):
        transformed_annotation_for_param = []
        for st in annotation_for_param:
            statement = st.split(':')
            st_len = len(statement)
            assert(st_len <= 3)

            transformed_statement = st
            if (statement[0] == "TaintOutput"):
                if (st_len == 3 and statement[2] != ""):
                    print("For TaintOutput in {} annotation elem #{} ignore data from cooddy annotations file".format(funcName, i))
                offset = statement[1] if st_len == 2 else ""
                transformed_statement = "TaintOutput:{}:UntrustedSource".format(offset)
            elif (statement[0] == "TaintPropagation"):
                if (st_len == 2):
                    print("TaintPropagation in {} annotation elem #{} misprint".format(funcName, i))
                    transformed_statement = "TaintPropagation::UntrustedSource:{}".format(statement[1])
                elif (st_len == 3):
                    if (statement[1] != ""):
                        print("For TaintPropagation in {} annotation elem #{} ignore offset from cooddy annotations file".format(funcName, i))
                    transformed_statement = "TaintPropagation::UntrustedSource:{}".format(statement[2])
            elif (statement[0] == "SensitiveDataSource"):
                if (st_len != 1):
                    print("For SensitiveDataSource in {} annotation elem #{} ignore offset and data from cooddy annotations file".format(funcName, i))
                transformed_statement = "TaintOutput::SensitiveDataSource"

            transformed_annotation_for_param.append(transformed_statement)
        transformed_annotation.append(transformed_annotation_for_param)

    return transformed_annotation    



def transform(utbot_json, coody_json, process_taint):
    for coody_name, annotation in coody_json.items():
        funcName, mangledName = getNames(coody_name)
        if (process_taint):
            annotation = transform_annotation_with_taint_statements(annotation, funcName)
        utbot_json[mangledName] = {"name": funcName, "annotation": annotation, "properties": []}


def main():
    parser = ArgumentParser(
        prog='cooddy_annotations.py',
        description='This script transforms .json annotations used by Cooddy static analyzer into annotations understandable by KLEE')
    parser.add_argument('filenames', metavar='Path', nargs='+', help="Cooddy annotation .json file path")
    parser.add_argument('Output', help="Target file path for produced KLEE annotation")
    parser.add_argument('--taint', action='store_true', help="Enable processing of annotations associated with taint analysis " + 
                                                        "(needed if you want to use taint analysis)")
    args = parser.parse_args()
    utbot_json = dict()
    for filename in args.filenames:
        with open(filename) as file:
            j = json.load(file)
        transform(utbot_json, j, args.taint)
    with open(args.Output, 'w') as file:
        json.dump(utbot_json, file, indent="  ")


if __name__ == "__main__":
    main()
