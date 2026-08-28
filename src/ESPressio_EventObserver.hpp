#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventEnums.hpp"

namespace ESPressio {

    namespace Event {

        /// <summary>Typed observer contract for receiving events without supplying an explicit callback.</summary>
        /// <typeparam name="EventType">Concrete event type received by the observer.</typeparam>
        /// <remarks>Observer registration is non-owning; implementations must remain alive until their listener handle is unregistered or destroyed.</remarks>
        template <class EventType>
        class IEventObserver : public virtual Observable::IObserver {
            public:
                virtual ~IEventObserver() = default;

                /// <summary>Handles a dispatched event matching this observer's registration.</summary>
                /// <param name="event">Dispatched event instance.</param>
                /// <param name="dispatchMethod">Queue/stack method through which the event was dispatched.</param>
                /// <param name="priority">Priority associated with this dispatch.</param>
                virtual void OnEvent(
                    EventType* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority
                ) = 0;

                /// <summary>Determines whether this observer is interested in a specific event when custom filtering is selected.</summary>
                /// <returns><c>true</c> when the event should be delivered.</returns>
                virtual bool IsInterestedInEvent(EventType* event) {
                    (void)event;
                    return true;
                }
        };

    }

}
