

#define EXAMPLE_FOUR 1

#ifdef EXAMPLE_ONE
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
#endif // EXAMPLE_ONE

#ifdef EXAMPLE_TWO

#include <any>
#include <cassert>
#include <memory>
#include <queue>
#include <typeindex>

#include <boost/sml.hpp>
#include <iostream>


class base {
public:
    virtual ~base() = default;

    template <typename TEvent>
    void enqueue_event(const TEvent& e) {
        events.emplace(std::make_any<TEvent>(e));
    }

    void process_all_events() {
        while (!events.empty()) {
            auto& ev = events.front();
            dispatch_event(ev);
            events.pop();
        }
    }

protected:
    virtual void dispatch_event(std::any& event) = 0;

private:
    std::queue<std::any> events;
};

namespace sml = boost::sml;

class TD {};

template <typename T = TD>
class top : public base {
    struct e1 {};
    struct e2 {};

    struct idle {};
    struct running {};

    struct nested {
        auto operator()() const {
            using namespace sml;
            return make_transition_table(
                *state<idle> +event<e1> / [](const e1&, base* self) {
                    std::cout << "on_e1\n";
                    self->enqueue_event(e2{});
                } = state<running>,
                state<running> +event<e2> / [](const e2&, base* self) {
                    std::cout << "on_e2\n";
                    } = X
                    );
        }
    };

public:
    top() : sm{ static_cast<base*>(this) } {}

    void process() {
        enqueue_event(e1{});
        process_all_events();
        assert(sm.is(sml::X));
    }

protected:
    void dispatch_event(std::any& event) override {
        if (event.type() == typeid(e1)) {
            sm.process_event(std::any_cast<e1>(event));
        }
        else if (event.type() == typeid(e2)) {
            sm.process_event(std::any_cast<e2>(event));
        }
    }

private:
    sml::sm<nested> sm;
};

int main() {
    top<> sm;
    sm.process();
}

#endif // EXAMPLE_TWO

#ifdef EXAMPLE_THREE
#include <variant>
#include <queue>
#include <iostream>
#include <cassert>
#include <boost/sml.hpp>
 
template <typename Derived, typename EventVariant>
class topdown {
public:
    void enqueue_event(const EventVariant& e) {
        events.push(e);
    }

    void process_all_events() {
        while (!events.empty()) {
            auto ev = events.front();
            static_cast<Derived*>(this)->dispatch_event(ev);
            events.pop();
        }
    }

private:
    std::queue<EventVariant> events;
};

namespace sml = boost::sml;

struct TD {}; 

template <typename T = TD>
class top;

struct e1 {};
struct e2 {};

using event_variant = std::variant<e1, e2>;

template <typename T>
class top : public topdown<top<T>, event_variant> {
    using base = topdown<top<T>, event_variant>;
    struct idle {};
    struct running {};

    struct nested {
        auto operator()() const {
            using namespace sml;
            return make_transition_table(
                *state<idle> +event<e1> / [](const e1&, base* self) {
                    std::cout << "on_e1\n";
                    self->enqueue_event(e2{});
                } = state<running>,

                state<running> +event<e2> / [](const e2&, base* self) {
                    std::cout << "on_e2\n";
                    } = X
                    );
        }
    };

public:
    top() : sm(static_cast<base*>(this)) {}

    void process() {
        this->enqueue_event(e1{});
        this->process_all_events();
        assert(sm.is(sml::X));
    }

    void dispatch_event(event_variant& ev) {
        std::visit([this](auto&& e) {
            sm.process_event(e);
            }, ev);
    }

private:
    sml::sm<nested> sm;
};

int main() {
    top<> sm;
    sm.process();
}


#endif // EXAMPLE_THREE

#ifdef EXAMPLE_FOUR
#include <variant>
#include <queue>
#include <iostream>
#include <cassert>
#include <boost/sml.hpp>

template <typename Derived, typename EventVariant>
class topdown {
public:
    void process_event(const EventVariant& e) {
        bool reentrant_call = !events.empty();
        events.push(e);
        if (!reentrant_call) {
            while (!events.empty()) {
                auto ev = events.front();
                static_cast<Derived*>(this)->dispatch_event(ev);
                events.pop();
            }
        }
    }

private:
    std::queue<EventVariant> events;
};

namespace sml = boost::sml;

struct TD {};

template <typename T = TD>
class top;

struct e1 {};
struct e2 {};

using event_variant = std::variant<e1, e2>;

template <typename T>
class top : public topdown<top<T>, event_variant> {
    using base = topdown<top<T>, event_variant>;
    struct idle {};
    struct running {};

    struct nested {
        auto operator()() const {
            using namespace sml;
            return make_transition_table(
                *state<idle> +event<e1> / [](const e1&, base* self) {
                    std::cout << "on_e1\n";
                    self->process_event(e2{});
                } = state<running>,

                state<running> +event<e2> / [](const e2&, base* self) {
                    std::cout << "on_e2\n";
                    } = X
                    );
        }
    };

public:
    top() : sm(static_cast<base*>(this)) {}

    void process() {
        this->process_event(e1{});
        assert(sm.is(sml::X));
    }

    void dispatch_event(event_variant& ev) {
        std::visit([this](auto&& e) {
            sm.process_event(e);
            }, ev);
    }

private:
    sml::sm<nested> sm;
};

int main() {
    top<> sm;
    sm.process();
}


#endif // EXAMPLE_FOUR
