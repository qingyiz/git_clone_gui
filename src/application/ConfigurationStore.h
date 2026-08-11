#pragma once

#include "core/CloneRequest.h"

#include <optional>

namespace gitclone {

class ConfigurationStore {
public:
    virtual ~ConfigurationStore() = default;

    virtual std::optional<CloneRequest> load() const = 0;
    virtual bool save(const CloneRequest &request) = 0;
};

} // namespace gitclone
