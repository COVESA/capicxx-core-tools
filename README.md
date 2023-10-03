### CommonAPI C++ Core Tools

##### Copyright
Copyright (C) 2015,2016 Bayerische Motoren Werke Aktiengesellschaft (BMW AG).
Copyright (C) 2015,2016 GENIVI Alliance, Inc.

This file is part of GENIVI Project IPC Common API C++.
Contributions are licensed to the GENIVI Alliance under one or more Contribution License Agreements or MPL 2.0.

##### License
This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

##### CommonAPI C++ Specification and User Guide
The specification document and the user guide can be found in the CommonAPI documentation directory of the CommonAPI-Tools project.

##### Further information
https://covesa.github.io/capicxx-core-tools/

##### Build Instructions for Linux

You can build all code generators by calling maven from the command-line. Open a console and change in the directory org.genivi.commonapi.core.releng of your CommonAPI-Tools directory. Then call:

```bash
mvn -Dtarget.id=org.genivi.commonapi.core.target clean verify
```

After the successful build you will find the commond-line generators archived in `org.genivi.commonapi.core.cli.product/target/products/commonapi_core_generator.zip` and the update-sites in `org.genivi.commonapi.core.updatesite/target`.
