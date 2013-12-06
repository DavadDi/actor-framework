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


#ifndef CPPA_SPAWN_HPP
#define CPPA_SPAWN_HPP

#include <type_traits>

#include "cppa/policy.hpp"
#include "cppa/scheduler.hpp"
#include "cppa/typed_actor.hpp"
#include "cppa/spawn_options.hpp"

#include "cppa/detail/proper_actor.hpp"

namespace cppa {

/** @cond PRIVATE */

constexpr bool unbound_spawn_options(spawn_options opts) {
    return !has_monitor_flag(opts) && !has_link_flag(opts);
}

/** @endcond */

/**
 * @ingroup ActorCreation
 * @{
 */

/**
 * @brief Spawns an actor of type @p Impl.
 * @param args Constructor arguments.
 * @tparam Impl Subtype of {@link event_based_actor} or {@link sb_actor}.
 * @tparam Options Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 */
template<class Impl, spawn_options Options = no_spawn_options, typename... Ts>
actor spawn(Ts&&... args) {
    static_assert(std::is_base_of<event_based_actor, Impl>::value,
                  "Impl is not a derived type of event_based_actor");
    static_assert(unbound_spawn_options(Options),
                  "top-level spawns cannot have monitor or link flag");
    static_assert(unbound_spawn_options(Options),
                  "top-level spawns cannot have monitor or link flag");
    using scheduling_policy = typename std::conditional<
                                  has_detach_flag(Options),
                                  policy::no_scheduling,
                                  policy::cooperative_scheduling
                              >::type;
    using priority_policy = typename std::conditional<
                                has_priority_aware_flag(Options),
                                policy::prioritizing,
                                policy::not_prioritizing
                            >::type;
    using resume_policy = typename std::conditional<
                              has_blocking_api_flag(Options),
                              typename std::conditional<
                                  has_detach_flag(Options),
                                  policy::no_resume,
                                  policy::context_switching_resume
                              >::type,
                              policy::event_based_resume
                          >::type;
    using invoke_policy = typename std::conditional<
                              has_blocking_api_flag(Options),
                              policy::nestable_invoke,
                              policy::sequential_invoke
                          >::type;
    using proper_impl = detail::proper_actor<Impl,
                                             scheduling_policy,
                                             priority_policy,
                                             resume_policy,
                                             invoke_policy>;
    auto ptr = make_counted<proper_impl>(std::forward<Ts>(args)...);
    /*
    scheduled_actor_ptr ptr;
    if (has_priority_aware_flag(Options)) {
        using derived = typename extend<Impl>::template with<threaded, prioritizing>;
        ptr = make_counted<derived>(std::forward<Ts>(args)...);
    }
    else if (has_detach_flag(Options)) {
        using derived = typename extend<Impl>::template with<threaded>;
        ptr = make_counted<derived>(std::forward<Ts>(args)...);
    }
    else ptr = make_counted<Impl>(std::forward<Ts>(args)...);
    return get_scheduler()->exec(Options, std::move(ptr));
    */
}

/**
 * @brief Spawns a new {@link actor} that evaluates given arguments.
 * @param args A functor followed by its arguments.
 * @tparam Options Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 */
template<spawn_options Options = no_spawn_options, typename... Ts>
actor spawn(Ts&&... args) {
    static_assert(sizeof...(Ts) > 0, "too few arguments provided");
    using base_class = typename std::conditional<
                           has_blocking_api_flag(Options),
                           detail::functor_based_blocking_actor,
                           detail::functor_based_actor
                       >::type;
    return spawn<base_class>(std::forward<Ts>(args)...);
    using impl = detail::proper_actor<untyped_actor,
                                      scheduling_policy,
                                      priority_policy,
                                      resume_policy,
                                      invoke_policy>;
    return make_counted<impl>();
}

/**
 * @brief Spawns a new actor that evaluates given arguments and
 *        immediately joins @p grp.
 * @param args A functor followed by its arguments.
 * @tparam Options Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 * @note The spawned has joined the group before this function returns.
 */
/*
template<spawn_options Options = no_spawn_options, typename... Ts>
actor spawn_in_group(const group_ptr& grp, Ts&&... args) {
    static_assert(sizeof...(Ts) > 0, "too few arguments provided");
    auto init_cb = [=](local_actor* ptr) {
        ptr->join(grp);
    };
    return eval_sopts(Options,
                      get_scheduler()->exec(Options,
                                            init_cb,
                                            std::forward<Ts>(args)...));
}
*/

/**
 * @brief Spawns an actor of type @p Impl that immediately joins @p grp.
 * @param args Constructor arguments.
 * @tparam Impl Subtype of {@link event_based_actor} or {@link sb_actor}.
 * @tparam Options Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 * @note The spawned has joined the group before this function returns.
 */
/*
template<class Impl, spawn_options Options, typename... Ts>
actor spawn_in_group(const group_ptr& grp, Ts&&... args) {
    auto ptr = make_counted<Impl>(std::forward<Ts>(args)...);
    ptr->join(grp);
    return eval_sopts(Options, get_scheduler()->exec(Options, ptr));
}

template<class Impl, spawn_options Options = no_spawn_options, typename... Ts>
typename Impl::typed_pointer_type spawn_typed(Ts&&... args) {
    static_assert(util::tl_is_distinct<typename Impl::signatures>::value,
                  "typed actors are not allowed to define "
                  "multiple patterns with identical signature");
    auto p = make_counted<Impl>(std::forward<Ts>(args)...);
    using result_type = typename Impl::typed_pointer_type;
    return result_type::cast_from(
        eval_sopts(Options, get_scheduler()->exec(Options, std::move(p)))
    );
}
*/

/*TODO:
template<spawn_options Options, typename... Ts>
typed_actor<typename detail::deduce_signature<Ts>::type...>
spawn_typed(const match_expr<Ts...>& me) {
    static_assert(util::conjunction<
                      detail::match_expr_has_no_guard<Ts>::value...
                  >::value,
                  "typed actors are not allowed to use guard expressions");
    static_assert(util::tl_is_distinct<
                      util::type_list<
                          typename detail::deduce_signature<Ts>::arg_types...
                      >
                  >::value,
                  "typed actors are not allowed to define "
                  "multiple patterns with identical signature");
    using impl = detail::default_typed_actor<
                     typename detail::deduce_signature<Ts>::type...
                 >;
    return spawn_typed<impl, Options>(me);
}

template<typename... Ts>
typed_actor<typename detail::deduce_signature<Ts>::type...>
spawn_typed(const match_expr<Ts...>& me) {
    return spawn_typed<no_spawn_options>(me);
}

template<typename T0, typename T1, typename... Ts>
auto spawn_typed(T0&& v0, T1&& v1, Ts&&... vs)
-> decltype(spawn_typed(match_expr_collect(std::forward<T0>(v0),
                                           std::forward<T1>(v1),
                                           std::forward<Ts>(vs)...))) {
    return spawn_typed(match_expr_collect(std::forward<T0>(v0),
                                          std::forward<T1>(v1),
                                          std::forward<Ts>(vs)...));
}
*/

/** @} */

} // namespace cppa

#endif // CPPA_SPAWN_HPP
