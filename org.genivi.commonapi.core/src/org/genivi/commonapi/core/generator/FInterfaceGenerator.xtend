/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FVersion
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants
import java.util.List
import org.franca.core.franca.FType
import org.franca.core.franca.FStructType

class FInterfaceGenerator {
	@Inject extension FTypeGenerator
	@Inject extension FTypeCommonAreaGenerator
	@Inject extension FrancaGeneratorExtensions

    def generateInterface(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        if(FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true").equals("true")) {
            fileSystemAccess.generateFile(fInterface.headerPath, IFileSystemAccess.DEFAULT_OUTPUT, fInterface.generateHeader(modelid, deploymentAccessor))
            if (fInterface.hasSourceFile)
                fileSystemAccess.generateFile(fInterface.sourcePath, IFileSystemAccess.DEFAULT_OUTPUT, fInterface.generateSource(modelid, deploymentAccessor))
        }
        else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fInterface.headerPath, IFileSystemAccess.DEFAULT_OUTPUT, PreferenceConstants::NO_CODE)
            if (fInterface.hasSourceFile)
                fileSystemAccess.generateFile(fInterface.sourcePath, IFileSystemAccess.DEFAULT_OUTPUT, PreferenceConstants::NO_CODE)
        }
    }

    def generateInstanceIds(FInterface fInterface, IFileSystemAccess fileSystemAccess,  List<String> deployedInstances) {
        if(FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true").equals("true")) {
            fileSystemAccess.generateFile(fInterface.instanceHeaderPath, IFileSystemAccess.DEFAULT_OUTPUT, fInterface.generateInstanceHeader(deployedInstances))
        } else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fInterface.instanceHeaderPath, IFileSystemAccess.DEFAULT_OUTPUT, PreferenceConstants::NO_CODE)
        }
    }

    def generateInstanceHeader(FInterface fInterface, List<String> deployedInstances) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName.toUpperCase»_INSTANCE_HPP_
        #define «fInterface.defineName.toUpperCase»_INSTANCE_HPP_

        #include <string>

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        «FOR instanceId : deployedInstances»
            const char * const «fInterface.elementName.replace('.', '_')»_«instanceId.replace('.', '_')» = "«instanceId»";
        «ENDFOR»

        const std::string «fInterface.elementName»_INSTANCES[] = {
            «FOR instanceId : deployedInstances»
                «fInterface.elementName.replace('.', '_')»_«instanceId.replace('.', '_')»«IF instanceId != deployedInstances.last»,«ENDIF»
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName.toUpperCase»_INSTANCE_HPP_
    '''

    def private generateHeader(FInterface fInterface, IResource modelid, PropertyAccessor deploymentAccessor) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName.toUpperCase»_HPP_
        #define «fInterface.defineName.toUpperCase»_HPP_

        «val libraryHeaders = new HashSet<String>»
        «val generatedHeaders = new HashSet<String>»
        «fInterface.getRequiredHeaderFiles(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «IF fInterface.base !== null»
            #include <«fInterface.base.headerPath»>
        «ENDIF»

        «startInternalCompilation»

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «endInternalCompilation»

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.elementName»«IF fInterface.base !== null»
            : virtual public «fInterface.base.getTypeCollectionName(fInterface)»«ENDIF» {
        public:
            virtual ~«fInterface.elementName»() { }

            static inline const char* getInterface();
            static inline CommonAPI::Version getInterfaceVersion();
            «fInterface.generateFTypeDeclarations(deploymentAccessor)»
            «fInterface.generateFConstDeclarations(deploymentAccessor)»
        };

        const char* «fInterface.elementName»::getInterface() {
            return ("«fInterface.fullyQualifiedNameWithVersion»");
        }

        CommonAPI::Version «fInterface.elementName»::getInterfaceVersion() {
            «val FVersion itsVersion = fInterface.version»
            «IF itsVersion !== null»
                return CommonAPI::Version(«itsVersion.major», «itsVersion.minor»);
            «ELSE»
                return CommonAPI::Version(0, 0);
            «ENDIF»
        }

        «FOR type : fInterface.types»
           «FTypeGenerator::generateComments(type, false)»
           «type.generateFTypeInlineImplementation(fInterface, deploymentAccessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        namespace CommonAPI {
            «fInterface.generateVariantComparators»
        }

        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName.toUpperCase»_HPP_
    '''



    def private generateSource(FInterface fInterface, IResource modelid, PropertyAccessor _accessor) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #include "«fInterface.headerFile»"

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        «FOR method : fInterface.methodsWithError»
            «method.errors.generateFTypeImplementation(method.containingInterface, _accessor)»
        «ENDFOR»

        «FOR type : fInterface.types»
           «IF(type.needsSourceComment)»
                «FTypeGenerator::generateComments(type,false)»
           «ENDIF»
           «type.generateFTypeImplementation(fInterface, _accessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
    '''

    def void getRequiredHeaderFiles(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        libraryHeaders.add('CommonAPI/Types.hpp')
        if (!fInterface.methods.filter[errors !== null].empty) {
            libraryHeaders.addAll('CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp')
        }
        if (!fInterface.managedInterfaces.empty) {
            generatedHeaders.add('set');
        }
        fInterface.types.forEach[addRequiredHeaders(generatedHeaders, libraryHeaders)]
        var Iterable<FMethod> errorMethods = fInterface.methods.filter[errors!==null]
        if(errorMethods.size!=0){
            libraryHeaders.addAll('CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp')
            errorMethods.forEach[
                if (errors.base !== null) {
                   errors.base.addRequiredHeaders(generatedHeaders, libraryHeaders)
                }
            ]
        }

        generatedHeaders.remove(fInterface.headerPath)
    }

    def private hasSourceFile(FInterface fInterface) {
        val hasTypeWithImplementation = fInterface.types.exists[hasImplementation]
        return (hasTypeWithImplementation)
    }

    def private needsSourceComment(FType _type) {
        if (_type instanceof FStructType) {
            return _type.isPolymorphic
        }
        return false
    }
}
