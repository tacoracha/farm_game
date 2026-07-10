#pragma once

#include <cstdint>

namespace farm {

enum class EntityDomain : std::uint8_t {
    Ranch,
};

enum class RanchEntityType : std::uint8_t {
    Animal,
    Facility,
};

class FarmEntity {
public:
    virtual ~FarmEntity() = default;

    virtual EntityDomain Domain() const noexcept = 0;

    int id = 0;
};

class RanchEntity : public FarmEntity {
public:
    EntityDomain Domain() const noexcept final { return EntityDomain::Ranch; }

    virtual RanchEntityType RanchType() const noexcept = 0;
    virtual bool HasReadyOutput() const noexcept = 0;
};

}  // namespace farm
