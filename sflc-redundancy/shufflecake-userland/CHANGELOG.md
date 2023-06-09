# Changelog
This is the changelog for `shufflecake-userland` which is part of the Shufflecake project.
Shufflecake is a plausible deniability (hidden storage) layer for Linux. See <https://www.shufflecake.net>.
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).



## [Unreleased]

- Adopt SemVer convention.
- Block device path as mandatory argument rather than asked interactively to user.
- Improved random slice selection algorithms.
- CLI using `argp.h`.
- Clean `src` tree.
- Skip randfill as optional argument rather than interactive choice.
 
 
 
## [0.2] - 2023-04-17

### Added

- Automatic procedural naming of open volumes.
- Support larger volumes with headers of variable size.
- Add interactive option to skip random filling during init.

### Fixed

- Compile correctly on Linux kernel 6.1.

### Changed

- Switch from `libsodium` to `libgcrypt`.
- Change header format.
- Change syntax of commands.
- Switch to Scrypt KDF.

### Removed

- Flag `--no-randfill`.



## [0.1] - 2022-11-10

This is the first release of `shufflecake-userland`

