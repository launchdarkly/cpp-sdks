#pragma once

/**
 * Interface for a data store that holds feature flag data and other SDK
 * properties in a serialized form.
 *
 * This interface should be used for platform-specific integrations that store
 * data somewhere other than in memory. The SDK defaults to using only
 * in-memory storage.
 * Each data item is uniquely identified by the combination of a "namespace"
 * and a "key", and has a string value. These are defined as follows:
 *
 * - Both the namespace and the key are non-empty string.
 * - Both the namespace and the key contain only alphanumeric characters,
 *   hyphens, and underscores.
 * - The namespace always starts with "LaunchDarkly".
 * - The value can be any string, including an empty string.
 *
 * The SDK assumes that the persistence is only being used by a single instance
 * of the SDK per SDK key (two different SDK instances, with 2 different SDK
 * keys could use the same persistence instance). It does not implement
 * read-through behavior. It reads values at SDK initialization or when
 * changing contexts.
 *
 * The SDK, with correct usage, will not have overlapping writes to the same
 * key. The Read/Write methods may not always be called from the same thread.
 *
 * This interface does not depend on the ability to list the contents of the
 * store or namespaces. This is to maintain the simplicity of implementing a
 * key-value store on many platforms.
 */
class IPersistence {
   public:
    /**
     * Add or update a value in the store. If the value cannot be set, then
     * the function should complete normally.
     *
     * @param storage_namespace The namespace for the data.
     * @param key The key for the data.
     * @param value The data to add or update.
     */
    virtual void Set(std::string storage_namespace,
                          std::string key,
                          std::string data) noexcept = 0;

    /**
     * Remove a value from the store. If the value cannot be removed, then
     * the function should complete normally.
     *
     * @param storage_namespace The namespace of the data.
     * @param key The key of the data.
     */
    virtual void Remove(std::string storage_namespace,
                             std::string key) noexcept = 0;

    /**
     * Attempt to read a value from the store.
     * @param storage_namespace The namespace of the data.
     * @param key The key of the data.
     *
     * @return The read value, or std::nullopt if the value does not exist
     * or could not be read.
     */
    virtual std::optional<std::string> Read(std::string storage_namespace,
                                            std::string key) noexcept = 0;
};
