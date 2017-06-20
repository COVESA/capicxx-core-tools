/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DE_BMW_EXAMPLES_TYPE_UTILS_H_
#define DE_BMW_EXAMPLES_TYPE_UTILS_H_

#include <type_traits>

template <typename SearchT, typename... T>
struct typeIdOf;

template <typename SearchT, typename T>
struct typeIdOf<SearchT, T> {

    static const std::size_t value = std::is_same<SearchT, T>::value ? 1 : -1;
};

template <typename SearchT, typename T1, typename... T>
struct typeIdOf<SearchT, T1, T...> {
    static const std::size_t value = std::is_same<SearchT, T1>::value ? sizeof...(T)+1 : typeIdOf<SearchT, T...>::value;
};

#endif // DE_BMW_EXAMPLES_TYPE_UTILS_H_
