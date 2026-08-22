module;

#include <filesystem>
#include <memory>
#include "qicon_fwd.h"

export module QIconResource;

import ResourceManager;

namespace fs = std::filesystem;

export class QIconResource : public Resource {
  public:
    static constexpr const char* name = "QIconResource";

    explicit QIconResource();
    explicit QIconResource(const fs::path& path);

    const QIcon& icon() const;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};
