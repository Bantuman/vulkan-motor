#pragma once

#include <cstdint>

#include <memory>
#include <string>
#include <unordered_map>

template <typename Asset>
class AssetCache {
	public:
		using Container = std::unordered_map<std::string, std::shared_ptr<Asset>>;

		template <typename Loader, typename... Args>
		std::shared_ptr<Asset> get_or_load(const std::string& key, Args&&... args) {
			if (auto it = m_assets.find(key); it == m_assets.end()) {
				if (auto asset = make_temp<Loader>(std::forward<Args>(args)...); asset) {
					m_assets[key] = asset;
					return asset;
				}
				else {
					return {};
				}
			}
			else {
				return it->second;
			}
		}

		std::shared_ptr<Asset> get(const std::string& key) {
			if (auto it = m_assets.find(key); it != m_assets.end()) {
				return it->second;
			}

			return {};
		}

		void set(const std::string& key, std::shared_ptr<Asset> value) {
			m_assets.emplace(std::make_pair(key, std::move(value)));
		}

		template <typename Loader, typename... Args>
		std::shared_ptr<Asset> make_temp(Args&&... args) const {
			return Loader{}.load(std::forward<Args>(args)...);
		}

		typename Container::iterator begin() {
			return m_assets.begin();
		}

		typename Container::iterator end() {
			return m_assets.end();
		}
	private:
		Container m_assets;
};

