/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator


import javax.inject.Inject
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FModelElement
import static org.genivi.commonapi.core.generator.FTypeGenerator.*
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FType
import org.franca.core.franca.FModel
import java.util.List

class FTypeCommonAreaGenerator {
	@Inject private extension FrancaGeneratorExtensions

	def private getClassNamespaceWithName(FModelElement child, String name, FModelElement parent, String parentName) {
        var reference = name
        if (parent != null && parent != child)
            reference = parentName + '::' + reference
        return reference
    }
    
    def private isFEnumerationType(FType fType) {
        return fType instanceof FEnumerationType
    }
    
    def private getFEnumerationType(FType fType) {
        return fType as FEnumerationType
    }
    
    def generateTypeWriters(FTypeCollection fTypes) '''
    	«FOR type: fTypes.types»
    		«IF type.isFEnumerationType»
    			«generateTypeWriterImplementation(type.getFEnumerationType, type.name, fTypes, fTypes.name)»
    		«ENDIF»
    	«ENDFOR»
    '''
	def private List<String> getNamespaceAsList(FModel fModel) {
        newArrayList(fModel.name.split("\\."))
    }
    
    def FModel getModel(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel)
        return (fModelElement.eContainer as FModelElement).model
    }
    
    def generateTypeWriterImplementation(FEnumerationType fEnumerationType, String enumerationName, FModelElement parent, String parentName) '''
        template<>
        struct BasicTypeWriter<«parent.model.namespaceAsList.join("::") + "::" + fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»> {
        	inline static void writeType (CommonAPI::TypeOutputStream& typeStream) {
        		typeStream.«fEnumerationType.generateTypeOutput»;
        	}
        };
    '''

    def private generateTypeOutput(FEnumerationType fEnumerationType) {
		if (fEnumerationType.backingType.primitiveTypeName.equals("int8_t")) {
			return "writeInt8EnumType()";			
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("int16_t")) {
			return "writeInt16EnumType()";
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("int32_t")) {
			return "writeInt32EnumType()";
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("int64_t")) {
			return "writeInt64EnumType()";
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("uint8_t")) {
			return "writeUInt8EnumType()";		
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("uint16_t")) {
			return "writeUInt16EnumType()";
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("uint32_t")) {
			return "writeUInt32EnumType()";
		} else if (fEnumerationType.backingType.primitiveTypeName.equals("uint64_t")) {
			return "writeUInt64EnumType()";
		}
	}
}