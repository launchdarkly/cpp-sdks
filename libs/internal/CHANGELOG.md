# Changelog

### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 1.4.0 to 1.5.0

## [0.5.3](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.5.2...launchdarkly-cpp-internal-v0.5.3) (2023-12-22)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 1.3.0 to 1.4.0

## [0.5.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.5.1...launchdarkly-cpp-internal-v0.5.2) (2023-12-21)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 1.2.0 to 1.3.0

## [0.5.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.5.0...launchdarkly-cpp-internal-v0.5.1) (2023-12-13)

### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 1.1.0 to 1.2.0

## [0.5.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.4.0...launchdarkly-cpp-internal-v0.5.0) (2023-12-04)


### Features

* server-side data system ([#304](https://github.com/launchdarkly/cpp-sdks/issues/304)) ([9a3737d](https://github.com/launchdarkly/cpp-sdks/commit/9a3737d09b1e1e57e5c7e6d30fb0c92f606d284c))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 1.0.0 to 1.1.0

## [0.4.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.3.0...launchdarkly-cpp-internal-v0.4.0) (2023-11-29)


### Features

* omit empty items from data model JSON serialization ([#309](https://github.com/launchdarkly/cpp-sdks/issues/309)) ([9141732](https://github.com/launchdarkly/cpp-sdks/commit/9141732e7ec1c42481e52f61da3d726740f17595))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.5.0 to 1.0.0

## [0.3.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.2.0...launchdarkly-cpp-internal-v0.3.0) (2023-10-23)


### Features

* server-side SDK  ([#160](https://github.com/launchdarkly/cpp-sdks/issues/160)) ([75eece3](https://github.com/launchdarkly/cpp-sdks/commit/75eece3a46870fdb6bf4384c112700558099c4d1))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.4.0 to 0.5.0

## [0.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.1.11...launchdarkly-cpp-internal-v0.2.0) (2023-10-13)


### Features

* clean up LD CMake variables & allow for OpenSSL dynamic link ([#255](https://github.com/launchdarkly/cpp-sdks/issues/255)) ([ed23c9a](https://github.com/launchdarkly/cpp-sdks/commit/ed23c9a347665529a09d18111bb9d3b699381728))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.3.7 to 0.4.0

## [0.1.11](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.1.10...launchdarkly-cpp-internal-v0.1.11) (2023-10-11)


### Bug Fixes

* treat warnings as errors in CI ([#253](https://github.com/launchdarkly/cpp-sdks/issues/253)) ([7f4f168](https://github.com/launchdarkly/cpp-sdks/commit/7f4f168f47619d7fa8b8952feade485261c69049))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.3.6 to 0.3.7

## [0.1.10](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.1.9...launchdarkly-cpp-internal-v0.1.10) (2023-09-21)


### Bug Fixes

* catch exception if en_US.utf8-locale missing when parsing datetime headers ([#251](https://github.com/launchdarkly/cpp-sdks/issues/251)) ([eb2a8f0](https://github.com/launchdarkly/cpp-sdks/commit/eb2a8f093996361541e11659165cbecc94c15346))

## [0.1.5](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-internal-v0.1.4...launchdarkly-cpp-internal-v0.1.5) (2023-06-30)


### Bug Fixes

* Fix compilation with boost 1.82. ([#157](https://github.com/launchdarkly/cpp-sdks/issues/157)) ([868e9a6](https://github.com/launchdarkly/cpp-sdks/commit/868e9a647487fa78b3316d2d8f6b2c6728903b48))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.3.1 to 0.3.2

## 0.1.0 (2023-05-24)


### Features

* Add support for logging configuration. ([#88](https://github.com/launchdarkly/cpp-sdks/issues/88)) ([6516711](https://github.com/launchdarkly/cpp-sdks/commit/651671100570a46135ed37219e2b6b55e2311b42))
* Add the ability to persist and restore flag configuration. ([#93](https://github.com/launchdarkly/cpp-sdks/issues/93)) ([c50e0d1](https://github.com/launchdarkly/cpp-sdks/commit/c50e0d15da0c449caade91df33c2a125298904cf))
* implement Identify method ([#89](https://github.com/launchdarkly/cpp-sdks/issues/89)) ([6ab8e82](https://github.com/launchdarkly/cpp-sdks/commit/6ab8e82522ae9eadb4a6c0db60b4d867da34c472))
* implement remaining config C bindings ([#90](https://github.com/launchdarkly/cpp-sdks/issues/90)) ([1b1e66a](https://github.com/launchdarkly/cpp-sdks/commit/1b1e66aee27b1e09e630072dbc5abed29f4de6a3))
* Include version in user-agent. ([#98](https://github.com/launchdarkly/cpp-sdks/issues/98)) ([a33daac](https://github.com/launchdarkly/cpp-sdks/commit/a33daac78b5e64c3419a4a97bf29b638b679784c))
* Reorganize code to better facilitate encapsulation. ([#87](https://github.com/launchdarkly/cpp-sdks/issues/87)) ([94f94ae](https://github.com/launchdarkly/cpp-sdks/commit/94f94aee4b8961a3001afd39f936e9c744fd9759))
* Update windows static builds. ([#103](https://github.com/launchdarkly/cpp-sdks/issues/103)) ([5d08380](https://github.com/launchdarkly/cpp-sdks/commit/5d0838099f7a99de49a604a9b5133325959705ff))
* Use object libraries. ([#99](https://github.com/launchdarkly/cpp-sdks/issues/99)) ([1d848e5](https://github.com/launchdarkly/cpp-sdks/commit/1d848e552def961a0468bfb6bab33cb1c4a86d3b))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-common bumped from 0.0.0 to 0.1.0
