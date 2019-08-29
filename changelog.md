# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v1.0.1]
### Changed
- Modified the way getBearing() and getDirection() worked. Both functions now require that the azimuth be passed to them as a parameter.
- Updated readme.md to reflect the above change.
- Updated /examples/bearing/bearing.ino to reflect above change.
- Updated /examples/direction/direction.ino to reflect above change.

### Fixed
- Corrections made to readme.md
- Renamed smoothing() to _smoothing() since this is a private function.
- Renamed writeReg() to _writeReg() since this is a private function.

## [v1.0.0]
### Added
- Documentation to readme.md
- Initial commit.

## [v0.0.2]
### Added
- Added getBearing() and getDirection() functions.

## [v0.0.1]
### Added
- Initial creation.