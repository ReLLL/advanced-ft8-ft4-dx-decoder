# Build and release

Read when: changing host integration, building a matching binary or publishing a release.

## Build requirements

Apple Silicon macOS, Xcode command-line tools, CMake, Python 3 and a matching SDR++Brown application. Header dependencies can be installed with `brew install cmake fftw volk`. Runtime libraries come from Brown's Frameworks directory.

The pinned host source is `679f48deee5299a199d15d30e6e785fac6054896` in [SDRPlusPlusBrown](https://github.com/sannysanoff/SDRPlusPlusBrown). The staging script copies the required source and applies FFT plan lifetime corrections; it preserves the upstream checkout.

```sh
git clone https://github.com/sannysanoff/SDRPlusPlusBrown.git /tmp/SDRPlusPlusBrown
python3 scripts/stage_sources.py --source /tmp/SDRPlusPlusBrown
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-release -j4
ctest --test-dir build-release --output-on-failure
DX_TEST_BUILD=build-release python3 tests/test_helper.py
```

The helper's FFTW dependency must resolve through `@rpath` to Brown's bundled library. CMake adjusts its install name after linking. No host bundle is re-signed.

## Release gate

1. Increment the CMake patch version for the next build. Update README, guide and changelog when behavior changes.
2. Run the native tests above and a separate `-DSANITIZE=ON` build/test. Verify synthetic FT8/FT4 decoding and bounded helper recovery.
3. Check the rendered plugin in an isolated Brown profile: zeroed session views, first-heard sort/order, right-aligned time values, history-stat switching, splitter, arc color, help and details. Test the release binaries on live RF in the normal profile.
4. Sanitize generated documentation. Audit source and assets for personal names, emails, credentials, profile data and local absolute paths. Credit the creator as ReLLL; use the GitHub noreply identity for commits.
5. Package with `python3 scripts/package_release.py --build build-release`. This copies only release binaries, installation files, map data, public documentation and license notices; ad-hoc signs only the staged plugin/helper; emits ZIP, corresponding source archive and checksums.
6. Review binary dependencies and deployment targets. Commit named source paths, push, tag `vX.Y.Z` and create the GitHub release with generated assets. Verify public visibility, creator identity and downloaded checksums.

A binary plugin is tied to the host ABI. Rebuilding for a different host requires compatible source/headers and corresponding tests. A lower Mach-O deployment target alone does not establish older-OS runtime compatibility; macOS 15 is the build minimum and macOS 26 is the tested environment for this release.
