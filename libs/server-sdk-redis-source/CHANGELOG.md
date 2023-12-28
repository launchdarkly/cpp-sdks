# Changelog

## [2.1.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-redis-source-v2.0.0...launchdarkly-cpp-server-redis-source-v2.1.0) (2023-12-26)


### Features

* add LDAllFlagsState_Map C binding ([#350](https://github.com/launchdarkly/cpp-sdks/issues/350)) ([2aca898](https://github.com/launchdarkly/cpp-sdks/commit/2aca898074b16cbb34498c289869b7687413df51))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-server bumped from 3.2.0 to 3.3.0

## [2.0.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-redis-source-v1.0.2...launchdarkly-cpp-server-redis-source-v2.0.0) (2023-12-22)


### ⚠ BREAKING CHANGES

* RedisDataSource::Create should return unique_ptr instead of shared_ptr ([#344](https://github.com/launchdarkly/cpp-sdks/issues/344))

### Features

* redis data source C bindings ([#345](https://github.com/launchdarkly/cpp-sdks/issues/345)) ([03b7de1](https://github.com/launchdarkly/cpp-sdks/commit/03b7de195febdcd4739d670448f5aefcbc2e9a2d))
* RedisDataSource::Create should return unique_ptr instead of shared_ptr ([#344](https://github.com/launchdarkly/cpp-sdks/issues/344)) ([07661c4](https://github.com/launchdarkly/cpp-sdks/commit/07661c4a8a6571fdf04d016f5bad5e69fb10216e))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-server bumped from 3.1.0 to 3.2.0

## [1.0.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-redis-source-v1.0.1...launchdarkly-cpp-server-redis-source-v1.0.2) (2023-12-21)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-server bumped from 3.0.1 to 3.1.0

## [1.0.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-redis-source-v1.0.0...launchdarkly-cpp-server-redis-source-v1.0.1) (2023-12-13)

### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-server bumped from 3.0.0 to 3.0.1

## [1.0.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-redis-source-v0.1.1...launchdarkly-cpp-server-redis-source-v1.0.0) (2023-12-12)


### Features

* redis source 1.0 ([#327](https://github.com/launchdarkly/cpp-sdks/issues/327)) ([152f139](https://github.com/launchdarkly/cpp-sdks/commit/152f139917356d262dfd84e518b0ba8c84d39765))

## 0.1.0 (2023-12-04)


### Features

* server-side data system ([#304](https://github.com/launchdarkly/cpp-sdks/issues/304)) ([9a3737d](https://github.com/launchdarkly/cpp-sdks/commit/9a3737d09b1e1e57e5c7e6d30fb0c92f606d284c))