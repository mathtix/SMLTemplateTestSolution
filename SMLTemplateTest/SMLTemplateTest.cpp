
// $CXX -std=c++14 nested.cpp
#include <boost/sml.hpp>
#include <cassert>
#include <iostream>

namespace sml = boost::sml;

class base
{
public:
    virtual ~base() = default;
    template <typename TEvent>
    void process_event(TEvent const& event);
};

class TD {};

template <class T = class TD>  // Use a dummy template to delay POI and support nested SM
class top : protected base
{
    friend class base;  // Allow base class to access private members of top
protected:
    struct e1 {};
    struct e2 {};


    struct idle {};
    struct running {};

    struct nested {
        auto operator()() const noexcept {
            using namespace sml;

            const auto on_e1 = [](e1 const& event, base* instance) {
                std::cout << "on_e1" << std::endl;
                instance->process_event(e2{});
                };

            const auto on_e2 = [](e2 const& event, base* instance) {
                std::cout << "on_e2" << std::endl;
                };

            return make_transition_table(
                sml::state<running> <= *sml::state<idle> + event<e1> / on_e1,
                sml::X              <= sml::state<running> + event<e2> / on_e2
            );
        }
    };

public:
    top();
    void process() {
        sm.process_event(e1{});
        assert(sm.is(sml::X));
    }


protected:
    sml::sm<nested> sm;
};

template <class T>
top<T>::top() : sm{ static_cast<base*>(this) }  // Pass the outer class instance to the state machine
{
}

template <typename TEvent>
void base::process_event(TEvent const& event) {
    std::cout << "doit()" << std::endl;
    static_cast<top<>*>(this)->sm.process_event(event);   
}

int main() {
    top<> sm{};
    sm.process();
}
