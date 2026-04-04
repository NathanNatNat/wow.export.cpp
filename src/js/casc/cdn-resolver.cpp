/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "cdn-resolver.h"
#include "../constants.h"
#include "../generics.h"
#include "../log.h"
#include "../core.h"
#include "version-config.h"

#include <algorithm>
#include <format>
#include <future>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace casc {
namespace cdn_resolver {

namespace {

/**
 * Manages CDN host resolution with intelligent pre-caching for performance.
 * Maintains a cache of regionTag + hostKey => bestHost mappings.
 */

struct CacheEntry {
	std::shared_future<std::vector<HostResult>> future;
	std::optional<HostResult> bestHost;
	std::vector<HostResult> rankedHosts;
};

// Map of cacheKey -> { future, bestHost, rankedHosts }
// cacheKey = region + '|' + hosts + '|' + fallback to handle different products with different hosts
std::mutex cacheMutex;
std::unordered_map<std::string, CacheEntry> resolutionCache;

// Track hosts that have failed to respond properly (e.g., censored responses)
std::unordered_set<std::string> failedHosts;

// Background threads for pre-resolution
std::vector<std::jthread> backgroundThreads;

/**
 * Generate cache key from region and hosts.
 * @param region Region tag
 * @param hosts Space-separated hosts
 * @return Cache key
 */
std::string _getCacheKey(const std::string& region, const std::string& hosts) {
	std::string fallback;
	if (core::view && core::view->config.is_object())
		fallback = core::view->config.value("cdnFallbackHosts", std::string(""));

	return region + "|" + hosts + "|" + fallback;
}

/**
 * Ping all hosts in server config and rank them by speed.
 * Excludes hosts that have previously failed.
 * @param region Region tag
 * @param serverConfig Server configuration
 * @return Array of hosts sorted by ping (fastest first)
 */
std::vector<HostResult> _resolveHosts(const std::string& region, const std::unordered_map<std::string, std::string>& serverConfig) {
	const auto& hostsStr = serverConfig.at("Hosts");
	logging::write(std::format("Resolving best host for {}: {}", region, hostsStr));

	std::vector<std::string> hosts;
	{
		std::istringstream iss(hostsStr);
		std::string token;
		while (iss >> token)
			hosts.push_back("https://" + token + "/");
	}

	std::string fallback_raw;
	if (core::view && core::view->config.is_object())
		fallback_raw = core::view->config.value("cdnFallbackHosts", std::string(""));

	{
		std::istringstream iss(fallback_raw);
		std::string token;
		while (std::getline(iss, token, ',')) {
			auto start = token.find_first_not_of(" \t");
			auto end = token.find_last_not_of(" \t");
			if (start == std::string::npos)
				continue;

			token = token.substr(start, end - start + 1);
			if (token.empty())
				continue;

			std::string fh = "https://" + token + "/";
			if (std::find(hosts.begin(), hosts.end(), fh) == hosts.end())
				hosts.push_back(fh);
		}
	}

	// Snapshot failed hosts under lock
	std::unordered_set<std::string> currentFailed;
	{
		std::lock_guard lock(cacheMutex);
		currentFailed = failedHosts;
	}

	std::vector<HostResult> validHosts;
	std::mutex validMutex;
	std::vector<std::future<void>> hostPings;

	for (const auto& host : hosts) {
		if (currentFailed.count(host)) {
			logging::write(std::format("Skipping previously failed host: {}", host));
			continue;
		}

		hostPings.push_back(std::async(std::launch::async, [&validHosts, &validMutex, host]() {
			try {
				auto ping = generics::ping(host);
				logging::write(std::format("Host {} resolved with {}ms ping", host, ping));
				std::lock_guard lock(validMutex);
				validHosts.push_back({ host, ping });
			} catch (const std::exception& e) {
				logging::write(std::format("Host {} failed to resolve a ping: {}", host, e.what()));
			} catch (...) {
				logging::write(std::format("Host {} failed to resolve a ping: unknown error", host));
			}
		}));
	}

	// Wait for all pings to complete (equivalent to Promise.allSettled)
	for (auto& f : hostPings)
		f.get();

	if (validHosts.empty())
		throw std::runtime_error("Unable to resolve any CDN hosts (all failed or blocked).");

	std::sort(validHosts.begin(), validHosts.end(), [](const HostResult& a, const HostResult& b) {
		return a.ping < b.ping;
	});

	logging::write(std::format("{} resolved as the fastest host with a ping of {}ms", validHosts[0].host, validHosts[0].ping));
	return validHosts;
}

/**
 * Start resolution for a specific region/product combination for pre-caching.
 * @param region Region tag
 * @param product Product name
 */
void _resolveRegionProduct(const std::string& region, const std::string& product) {
	try {
		std::string host;
		if (region == "cn")
			host = std::string(constants::PATCH::HOST_CHINA);
		else
			host = "https://" + region + ".version.battle.net/";

		std::string url = host + product + std::string(constants::PATCH::SERVER_CONFIG);
		auto data = generics::get(url);

		std::string text(data.begin(), data.end());
		auto serverConfigs = casc::parseVersionConfig(text);
		auto serverConfig = std::find_if(serverConfigs.begin(), serverConfigs.end(),
			[&region](const std::unordered_map<std::string, std::string>& e) {
				auto nameIt = e.find("Name");
				return nameIt != e.end() && nameIt->second == region;
			});

		if (serverConfig == serverConfigs.end())
			throw std::runtime_error("CDN config does not contain entry for region " + region);

		// Use getBestHost to resolve and cache
		getBestHost(region, *serverConfig);
	} catch (const std::exception& error) {
		logging::write(std::format("Failed to pre-resolve CDN hosts for region {}: {}", region, error.what()));
	}
}

} // anonymous namespace

void startPreResolution(const std::string& region, const std::string& product) {
	logging::write(std::format("Starting CDN pre-resolution for region: {}", region));
	backgroundThreads.emplace_back([region, product]() {
		_resolveRegionProduct(region, product);
	});
}

std::string getBestHost(const std::string& region, const std::unordered_map<std::string, std::string>& serverConfig) {
	auto cacheKey = _getCacheKey(region, serverConfig.at("Hosts"));

	std::shared_future<std::vector<HostResult>> existingFuture;
	bool isNewResolution = false;

	{
		std::lock_guard lock(cacheMutex);
		auto it = resolutionCache.find(cacheKey);
		if (it != resolutionCache.end()) {
			auto& entry = it->second;

			if (entry.bestHost) {
				logging::write(std::format("Using cached CDN host for {}: {}", region, entry.bestHost->host));
				return entry.bestHost->host + serverConfig.at("Path") + "/";
			}

			if (entry.future.valid()) {
				logging::write(std::format("Waiting for CDN resolution for {}", region));
				existingFuture = entry.future;
			}
		}

		if (!existingFuture.valid()) {
			logging::write(std::format("Resolving CDN hosts for {}: {}", region, serverConfig.at("Hosts")));

			auto future = std::async(std::launch::async, [region, serverConfig]() {
				return _resolveHosts(region, serverConfig);
			}).share();

			resolutionCache[cacheKey] = CacheEntry{ future, std::nullopt, {} };
			existingFuture = future;
			isNewResolution = true;
		}
	}

	auto rankedHosts = existingFuture.get();

	if (isNewResolution) {
		std::lock_guard lock(cacheMutex);
		auto& entry = resolutionCache[cacheKey];
		entry.bestHost = rankedHosts[0];
		entry.rankedHosts = rankedHosts;
	}

	return rankedHosts[0].host + serverConfig.at("Path") + "/";
}

std::vector<std::string> getRankedHosts(const std::string& region, const std::unordered_map<std::string, std::string>& serverConfig) {
	auto cacheKey = _getCacheKey(region, serverConfig.at("Hosts"));

	std::shared_future<std::vector<HostResult>> existingFuture;
	bool isNewResolution = false;

	{
		std::lock_guard lock(cacheMutex);
		auto it = resolutionCache.find(cacheKey);
		if (it != resolutionCache.end()) {
			auto& entry = it->second;

			if (!entry.rankedHosts.empty()) {
				logging::write(std::format("Using cached ranked CDN hosts for {}", region));
				std::vector<std::string> result;
				for (const auto& h : entry.rankedHosts)
					result.push_back(h.host + serverConfig.at("Path") + "/");
				return result;
			}

			if (entry.future.valid()) {
				logging::write(std::format("Waiting for CDN resolution for {}", region));
				existingFuture = entry.future;
			}
		}

		if (!existingFuture.valid()) {
			logging::write(std::format("Resolving CDN hosts for {}: {}", region, serverConfig.at("Hosts")));

			auto future = std::async(std::launch::async, [region, serverConfig]() {
				return _resolveHosts(region, serverConfig);
			}).share();

			resolutionCache[cacheKey] = CacheEntry{ future, std::nullopt, {} };
			existingFuture = future;
			isNewResolution = true;
		}
	}

	auto rankedHosts = existingFuture.get();

	if (isNewResolution) {
		std::lock_guard lock(cacheMutex);
		auto& entry = resolutionCache[cacheKey];
		entry.bestHost = rankedHosts[0];
		entry.rankedHosts = rankedHosts;
	}

	std::vector<std::string> result;
	for (const auto& h : rankedHosts)
		result.push_back(h.host + serverConfig.at("Path") + "/");
	return result;
}

void markHostFailed(const std::string& host) {
	logging::write(std::format("Marking CDN host as failed: {}", host));
	std::lock_guard lock(cacheMutex);
	failedHosts.insert(host);
}

} // namespace cdn_resolver
} // namespace casc