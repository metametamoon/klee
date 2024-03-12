install_binary_artifact_z3 () {
  set +e
  brew install z3
  export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${BASE}/z3-4.12.4-install/lib/"
}

is_installed_z3() {
    [[ -f "/opt/homebrew/bin/z3" ]]
}