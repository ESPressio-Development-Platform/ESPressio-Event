#pragma once

#include <ESPressio_IObserver.hpp>

#include "ESPressio_EventEnums.hpp"
#include "ESPressio_EventTypes.hpp"

namespace ESPressio {

    namespace Event {

        /// <summary>Typed observer contract for receiving Events without supplying an explicit callback.</summary>
        /// <typeparam name="EventType">Concrete Event type received by the observer.</typeparam>
        /// <remarks>Observer registration is non-owning; implementations must remain alive until their listener handle is unregistered or destroyed.</remarks>
        template <class EventType>
        class IEventObserver : public virtual Observable::IObserver {
            public:
                virtual ~IEventObserver() = default;

                /// <summary>Handles a dispatched Event matching this observer's registration.</summary>
                /// <param name="event">Dispatched Event instance.</param>
                /// <param name="dispatchMethod">Queue/stack method through which the Event was dispatched.</param>
                /// <param name="priority">Application/local-dispatch priority associated with this dispatch.</param>
                /// <param name="context">Transport-independent local/remote provenance for this dispatch.</param>
                virtual void OnEvent(
                    EventType* event,
                    EventDispatchMethod dispatchMethod,
                    EventPriority priority,
                    const EventDispatchContext& context
                ) = 0;

                /// <summary>Determines whether this observer is interested in a specific Event when custom filtering is selected.</summary>
                /// <returns><c>true</c> when the Event should be delivered.</returns>
                virtual bool IsInterestedInEvent(EventType* event) {
                    (void)event;
                    return true;
                }
        };

    }

}
