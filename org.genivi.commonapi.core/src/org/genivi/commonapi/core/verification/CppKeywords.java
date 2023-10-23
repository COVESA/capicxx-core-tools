/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.verification;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class CppKeywords {

    public List<String> keyWords = new ArrayList<String>();

    public HashMap<String, String> handleReservedWords = new HashMap<String, String>();

    public CppKeywords() {

        keyWords.add("alignas");
        keyWords.add("alignof");
        keyWords.add("and");
        keyWords.add("and_eq");
        keyWords.add("asm");
        keyWords.add("auto");
        keyWords.add("bitand");
        keyWords.add("bitor");
        keyWords.add("bool");
        keyWords.add("break");
        keyWords.add("case");
        keyWords.add("catch");
        keyWords.add("char");
        keyWords.add("char16_t");
        keyWords.add("char32_t");
        keyWords.add("class");
        keyWords.add("compl");
        keyWords.add("const");
        keyWords.add("constexpr");
        keyWords.add("const_cast");
        keyWords.add("continue");
        keyWords.add("decltype");
        keyWords.add("default");
        keyWords.add("delete");
        keyWords.add("do");
        keyWords.add("double");
        keyWords.add("dynamic_cast");
        keyWords.add("else");
        keyWords.add("enum");
        keyWords.add("explicit");
        keyWords.add("export");
        keyWords.add("extern");
        keyWords.add("false");
        keyWords.add("float");
        keyWords.add("for");
        keyWords.add("friend");
        keyWords.add("goto");
        keyWords.add("if");
        keyWords.add("inline");
        keyWords.add("int");
        keyWords.add("long");
        keyWords.add("mutable");
        keyWords.add("namespace");
        keyWords.add("new");
        keyWords.add("noexcept");
        keyWords.add("not");
        keyWords.add("not_eq");
        keyWords.add("nullptr");
        keyWords.add("operator");
        keyWords.add("or");
        keyWords.add("or_eq");
        keyWords.add("private");
        keyWords.add("protected");
        keyWords.add("public");
        keyWords.add("register");
        keyWords.add("reinterpret_cast");
        keyWords.add("return");
        keyWords.add("short");
        keyWords.add("signed");
        keyWords.add("sizeof");
        keyWords.add("static");
        keyWords.add("static_assert");
        keyWords.add("static_cast");
        keyWords.add("struct");
        keyWords.add("switch");
        keyWords.add("template");
        keyWords.add("this");
        keyWords.add("thread_local");
        keyWords.add("throw");
        keyWords.add("true");
        keyWords.add("try");
        keyWords.add("typedef");
        keyWords.add("typeid");
        keyWords.add("typename");
        keyWords.add("union");
        keyWords.add("unsigned");
        keyWords.add("using");
        keyWords.add("virtual");
        keyWords.add("void");
        keyWords.add("volatile");
        keyWords.add("wchar_t");
        keyWords.add("while");
        keyWords.add("xor");
        keyWords.add("xor_eq");

        handleReservedWords.put("AnonymousTypeCollection", "AnonymousTypeCollection_");
        handleReservedWords.put("instance", "instance_");
        handleReservedWords.put("attributes", "attributes_");
        handleReservedWords.put("info", "info_");
        handleReservedWords.put("callback", "callback_");
        handleReservedWords.put("internalCallStatus", "internalCallStatus_");
        handleReservedWords.put("new", "new_");
        handleReservedWords.put("client", "client_");
        handleReservedWords.put("InterfaceVersion", "ETSInterfaceVersion"); // This attribute's name is already used to access version information of the remote
    }
}
