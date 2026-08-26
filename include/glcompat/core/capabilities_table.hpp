#pragma once

#include "capabilities.hpp"
#include <array>
#include <string>

namespace glcompat {

// Concrete capability table. Backends populate it once during initialization
// from backend/version/extension detection, then the frontend queries it.
class CapabilityTable : public ICapabilities {
public:
    CapabilityTable() {
        table_.fill(FeatureSupport::Unsupported);
    }

    void set(Feature feature, FeatureSupport support) {
        table_[static_cast<size_t>(feature)] = support;
    }

    FeatureSupport getFeatureSupport(Feature feature) const override {
        return table_[static_cast<size_t>(feature)];
    }

    std::string featureName(Feature feature) const override;

private:
    std::array<FeatureSupport, static_cast<size_t>(Feature::FeatureCount)> table_;
};

} // namespace glcompat
