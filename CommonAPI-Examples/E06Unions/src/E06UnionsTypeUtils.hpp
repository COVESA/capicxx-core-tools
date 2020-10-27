// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E05_UNIONS_TYPE_UTILS_HPP_
#define E05_UNIONS_TYPE_UTILS_HPP_

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

#endif // E05_UNIONS_TYPE_UTILS_HPP_
