#ifndef OVERLOADED_H
#define OVERLOADED_H

// Assemble des lambdas en un visiteur pour `std::visit` : un type du variant sans lambda ne compile pas.
template <class... Visitors>
struct Overloaded : Visitors...
{
    using Visitors::operator()...;
};

template <class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

#endif
