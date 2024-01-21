#ifndef SPOT_BASE_COPYABLE_H
#define SPOT_BASE_COPYABLE_H

namespace spot
{

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class Copyable
{
};

};

#endif  // FTMD_BASE_COPYABLE_H
