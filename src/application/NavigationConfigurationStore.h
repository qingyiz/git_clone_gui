#pragma once

#include <optional>

namespace gitclone {

enum class NavigationPage {
    Clone,
    Workspace
};

class NavigationConfigurationStore {
public:
    virtual ~NavigationConfigurationStore() = default;

    virtual std::optional<NavigationPage> loadCurrentPage() const = 0;
    virtual bool saveCurrentPage(NavigationPage page) = 0;
};

} // namespace gitclone
