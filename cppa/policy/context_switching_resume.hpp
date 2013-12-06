/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef CPPA_CONTEXT_SWITCHING_ACTOR_HPP
#define CPPA_CONTEXT_SWITCHING_ACTOR_HPP

#include "cppa/config.hpp"
#include "cppa/mailbox_element.hpp"

#include "cppa/util/fiber.hpp"
#include "cppa/detail/yield_interface.hpp"

namespace cppa { class local_actor; }

namespace cppa { namespace policy {

/**
 * @brief Context-switching actor implementation.
 * @extends scheduled_actor
 */
class context_switching_resume {

 public:



    template<class Actor, typename F>
    void fetch_messages(Actor* self, F cb) {
        auto e = self->m_mailbox.try_pop();
        while (e == nullptr) {
            if (self->m_mailbox.can_fetch_more() == false) {
                self->set_state(actor_state::about_to_block);
                // make sure mailbox is empty
                if (self->m_mailbox.can_fetch_more()) {
                    // someone preempt us => continue
                    self->set_state(actor_state::ready);
                }
                // wait until actor becomes rescheduled
                else detail::yield(detail::yield_state::blocked);
            }
        }
        // ok, we have at least one message
        while (e) {
            cb(e);
            e = self->m_mailbox.try_pop();
        }
    }

    template<class Actor, typename F>
    void try_fetch_messages(Actor* self, F cb) {
        auto e = self->m_mailbox.try_pop();
        while (e) {
            cb(e);
            e = self->m_mailbox.try_pop();
        }
    }

    template<class Actor>
    resume_result resume(Actor* self, util::fiber* from) {
        CPPA_LOG_TRACE("state = " << static_cast<int>(self->state()));
        CPPA_REQUIRE(from != nullptr);
        CPPA_REQUIRE(next_job == nullptr);
        using namespace detail;
        for (;;) {
            switch (call(&m_fiber, from)) {
                case yield_state::done: {
                    CPPA_REQUIRE(next_job == nullptr);
                    return resume_result::actor_done;
                }
                case yield_state::ready: {
                    break;
                }
                case yield_state::blocked: {
                    CPPA_REQUIRE(next_job == nullptr);
                    CPPA_REQUIRE(m_chained_actor == nullptr);
                    switch (compare_exchange_state(actor_state::about_to_block,
                                                   actor_state::blocked)) {
                        case actor_state::ready: {
                            // restore variables
                            CPPA_REQUIRE(next_job == nullptr);
                            break;
                        }
                        case actor_state::blocked: {
                            // wait until someone re-schedules that actor
                            return resume_result::actor_blocked;
                        }
                        default: {
                            CPPA_CRITICAL("illegal yield result");
                        }
                    }
                    break;
                }
                default: {
                    CPPA_CRITICAL("illegal state");
                }
            }
        }
    }

 private:

    // required by util::fiber
    static void trampoline(void* _this);

    // members
    util::fiber m_fiber;

};

} } // namespace cppa::policy

#endif // CPPA_CONTEXT_SWITCHING_ACTOR_HPP
