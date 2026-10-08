#pragma once

// Typed parameters for one feature's recognition. The core forwards these
// but never interprets them (SPEC section 3.4): only the recognizer that
// owns the concrete type casts back to it. Type knowledge lives at the two
// ends (UI and recognizer), never in the middle.
class IParameters
{
public:
    virtual ~IParameters() = default;
};
