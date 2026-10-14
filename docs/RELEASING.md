# Build and release

Read when: changing host integration, building a matching binary or publishing a release.

## Build requirements

Apple Silicon macOS, Xcode command-line tools, CMake, Python 3 and matching SDR++ and SDR++Brown applications. Header dependencies can be installed with `brew install cmake fftw volk`. Each build uses its selected host's Frameworks directory.

The pinned host source is `679f48deee5299a199d15d30e6e785fac6054896` in [SDRPlusPlusBrown](https://github.com/sannysanoff/SDRPlusPlusBrown). The staging script copies the required source and applies FFT plan lifetime corrections; it preserves the upstream checkout.

```sh
git clone https://github.com/sannysanoff/SDRPlusPlusBrown.git /tmp/SDRPlusPlusBrown
python3 scripts/stage_sources.py --source /tmp/SDRPlusPlusBrown
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-release -j4
ctest --test-dir build-release --output-on-failure
DX_TEST_BUILD=build-release python3 tests/test_helper.py
```

For standard SDR++, stage the matching source without changing the upstream checkout:

```sh
git clone https://github.com/AlexandreRouma/SDRPlusPlus.git /tmp/SDRPlusPlus
mkdir -p build-sdrpp-source
git -C /tmp/SDRPlusPlus archive 762444d3406d68fbc9d4dff8cc237d382793a6e3 core license | tar -x -C build-sdrpp-source
echo 762444d3406d68fbc9d4dff8cc237d382793a6e3 > build-sdrpp-source/source-pin.txt
cmake -S . -B build-sdrpp -DDX_HOST=sdrpp -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-sdrpp -j4
ctest --test-dir build-sdrpp --output-on-failure
DX_TEST_BUILD=build-sdrpp python3 tests/test_helper.py
```

The standard host is SDR++ 1.2.0, May 28, 2024 build. Brown is 1.2.1, August 4, 2026 build. Standard integration uses standard APIs and ImGui hooks, independent receiver settings and bundled CTY.DAT; Brown's native integration stays intact. Both use the pinned MSHV decoder source. The helper's FFTW dependency must resolve through `@rpath` to its selected host's bundled library. CMake adjusts its install name after linking. No host bundle is re-signed.

## Release gate

1. Increment the CMake patch version for the next build. Update README, guide and changelog when behavior changes.
2. Run the native tests above and a separate `-DSANITIZE=ON` build/test. Verify synthetic FT8/FT4 decoding and bounded helper recovery.
3. Check rendered behavior and live RF using isolated profiles for both hosts. Preserve the normal profiles. Standard SDR++ config must use supported host keys: receiverGrid/receiverCall belong inside the plugin's module instance. Do not inject unsupported top-level centerFrequency/viewBandwidth keys; the older host can crash during config repair before any plugin loads.
4. Sanitize generated documentation. Audit source and assets for personal names, emails, credentials, profile data and local absolute paths. Credit the creator as ReLLL; use the GitHub noreply identity for commits.
5. Package with `python3 scripts/package_release.py --build build-release --sdrpp-build build-sdrpp`. Every package automatically compiles/runs the installer safety checks below. The local package builds the combined native installer, signs staged code ad hoc, verifies dependencies, and emits a DMG, ZIP, corresponding source and checksums. Host/version sidecars prevent mixing the two builds. Python and Xcode are build-time tools; customers need neither.
6. For a public installer, add `--signing-identity "$DX_SIGNING_IDENTITY" --notary-profile "$DX_NOTARY_PROFILE"`. Use a Developer ID Application certificate and a saved notarytool Keychain profile. Never put credentials in source. Signing uses hardened runtime; the helper disables library validation because it loads the host's independently signed libraries. The app and DMG must receive Apple's Accepted status, have stapled tickets, and pass Gatekeeper assessment. Failures stop packaging. Oz approved certificate-name visibility for this installer; visible app/repository credits remain ReLLL.
7. Review binary dependencies and deployment targets. Commit named source paths, push, tag `vX.Y.Z` and create the GitHub release with generated assets. Verify public visibility, creator identity and downloaded checksums.

A binary plugin is tied to the host ABI. Rebuilding for a different host requires compatible source/headers and corresponding tests. A lower Mach-O deployment target alone does not establish older-OS runtime compatibility; macOS 15 is the build minimum and macOS 26 is the tested environment for this release.

## Installer checks

```sh
mkdir -p build-installer
xcrun swiftc -O -target arm64-apple-macos15.0 installer/InstallerCore.swift tests/installer_tests.swift -o build-installer/installer-tests
build-installer/installer-tests
```

Checks use isolated temporary profiles and synthetic hosts: fresh installation, repeated upgrades, settings/log preservation, executable permissions, backups, rollback at staged/plugin/configuration replacement, running host, corrupt payloads, incompatible host, malformed configuration, symlink refusal, concurrent settings changes, standard payload/country data and receiver setup. Invalid locators must fail before quitting a running host. Keep normal profiles untouched during installer tests.

Inspect the packaged installer window, host selection and Cancel behavior without installing into a normal profile. Inspect both payload manifests/signatures, check the DMG, and verify downloaded release checksums. Each host fingerprint comes from its matched installed core library. Custom profiles are supported; binaries expect `/Applications/SDR++Brown.app` or `/Applications/SDR++.app` respectively. First-run preparation opens the selected host, waits for its default config, then asks it to quit normally. Existing disabled/enabled preferences are preserved on upgrades; the initial registration is enabled.
