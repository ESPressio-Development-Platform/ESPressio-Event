#pragma once

#include <cstddef>
#include <functional>

namespace ESPressio {

    namespace Event {

        /// <summary>Priority used to order pending Event dispatch work.</summary>
/**
 * ESPressio Memory Audit
 * Underlying storage: 4 bytes
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum EventPriority {
            Low = 0,
            Normal = 1,
            High = 2
        };


        /// <summary>Advances to the next Event priority, wrapping after High.</summary>
        inline EventPriority&
        operator++(
            EventPriority& priority
        ) {
            priority =
                static_cast<EventPriority>(
                    (
                        static_cast<int>(
                            priority
                        ) +
                        1
                    ) %
                    3
                );

            return priority;
        }


        /// <summary>Moves to the previous Event priority, wrapping before Low.</summary>
        inline EventPriority&
        operator--(
            EventPriority& priority
        ) {
            priority =
                static_cast<EventPriority>(
                    (
                        static_cast<int>(
                            priority
                        ) +
                        2
                    ) %
                    3
                );

            return priority;
        }


        /// <summary>Determines how an Event listener decides whether a matching Event should invoke its callback.</summary>
/**
 * ESPressio Memory Audit
 * Underlying storage: 4 bytes
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum EventListenerInterest {
            All,
            YoungerThan,
            Custom
        };


        /// <summary>Advances to the next listener-interest mode, wrapping after Custom.</summary>
        inline EventListenerInterest&
        operator++(
            EventListenerInterest& interest
        ) {
            interest =
                static_cast<EventListenerInterest>(
                    (
                        static_cast<int>(
                            interest
                        ) +
                        1
                    ) %
                    3
                );

            return interest;
        }


        /// <summary>Moves to the previous listener-interest mode, wrapping before All.</summary>
        inline EventListenerInterest&
        operator--(
            EventListenerInterest& interest
        ) {
            interest =
                static_cast<EventListenerInterest>(
                    (
                        static_cast<int>(
                            interest
                        ) +
                        2
                    ) %
                    3
                );

            return interest;
        }


        /// <summary>Selects LIFO stack or FIFO queue dispatch semantics for an Event.</summary>
/**
 * ESPressio Memory Audit
 * Underlying storage: 4 bytes
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum EventDispatchMethod {
            Stack,
            Queue
        };


        /// <summary>Toggles to the other Event dispatch method.</summary>
        inline EventDispatchMethod&
        operator++(
            EventDispatchMethod& method
        ) {
            method =
                static_cast<EventDispatchMethod>(
                    (
                        static_cast<int>(
                            method
                        ) +
                        1
                    ) %
                    2
                );

            return method;
        }


        /// <summary>Toggles to the other Event dispatch method.</summary>
        inline EventDispatchMethod&
        operator--(
            EventDispatchMethod& method
        ) {
            method =
                static_cast<EventDispatchMethod>(
                    (
                        static_cast<int>(
                            method
                        ) +
                        1
                    ) %
                    2
                );

            return method;
        }

    }

}


namespace std {

    template<>
    struct hash<
        ESPressio::Event::
            EventPriority
    > {
        std::size_t operator()(
            const ESPressio::Event::
                EventPriority& priority
        ) const noexcept {
            return
                std::hash<int>()(
                    static_cast<int>(
                        priority
                    )
                );
        }
    };

}
