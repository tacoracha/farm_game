#pragma once

#include "FeedMill.h"

namespace farm {

// V1: single FeedMill; future workshops can be added here.
class WorkshopSystem {
public:
    void Tick(int current_tick);

    FeedMill& GetFeedMill() { return feed_mill_; }
    const FeedMill& GetFeedMill() const { return feed_mill_; }

    void WriteStatus(std::ostream& os) const;

private:
    FeedMill feed_mill_;
};

}  // namespace farm
