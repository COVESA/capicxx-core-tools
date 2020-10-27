/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.verification

import java.util.Collection
import org.franca.deploymodel.dsl.fDeploy.FDModel
import java.util.ArrayList
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.deploymodel.dsl.fDeploy.FDTypes
import org.franca.deploymodel.dsl.fDeploy.FDElement
import org.franca.deploymodel.dsl.fDeploy.FDCompound
import org.franca.deploymodel.dsl.fDeploy.FDAttribute
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.eclipse.emf.common.util.Diagnostic
import org.eclipse.xtext.validation.FeatureBasedDiagnostic
import org.franca.deploymodel.dsl.fDeploy.FDArgument
import org.franca.deploymodel.dsl.fDeploy.FDField
import org.franca.deploymodel.dsl.fDeploy.FDEnumeration
import org.franca.deploymodel.dsl.fDeploy.FDOverwriteElement
import org.franca.deploymodel.dsl.fDeploy.FDCompoundOverwrites
import org.franca.core.franca.FCompoundType
import org.franca.deploymodel.dsl.fDeploy.FDProperty
/*
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FField
*/
import java.math.BigInteger
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import org.franca.deploymodel.dsl.fDeploy.FDGeneric
import org.franca.deploymodel.dsl.fDeploy.FDEnumerator
import java.util.HashMap
import org.eclipse.emf.common.util.BasicDiagnostic

class DeploymentValidator {
    var BasicDiagnostic diagnostics

    var fdInterfaces = new ArrayList<FDInterface>
    var fdTypeCollections = new ArrayList<FDTypes>
    var allFDepls = new ArrayList<FDModel> // all FDModels but without "_deployment_spec.fdepl" files
    var maxEnumValues = new HashMap<FEnumerationType, BigInteger>
    val DEPLOYMENT_SPECIFICATION_FILE_SUFFIX = "_deployment_spec.fdepl"
    val CORE_SPECIFICATION_TYPE = "core.deployment"

    def validate(Collection<FDModel> fdepls, BasicDiagnostic diagnostics) {
        this.diagnostics = diagnostics
        for (fdepl : fdepls) {
			addModel(fdepl)
        }
        validateEnumSizeDeployments
    }
    def private addModel(FDModel fdepl) {
    	var deplFileName = fdepl.eResource.URI.lastSegment
        if (!deplFileName.endsWith(DEPLOYMENT_SPECIFICATION_FILE_SUFFIX)) {
            allFDepls.add(fdepl)
            for (fdInterface : fdepl.deployments.filter(typeof(FDInterface))) {
                if (fdInterface.spec.name !== null && fdInterface.spec.name.contains(CORE_SPECIFICATION_TYPE))
                    fdInterfaces.add(fdInterface)
            }

            for (fdTypes : fdepl.deployments.filter(typeof(FDTypes))) {
                if (fdTypes.spec.name !== null && fdTypes.spec.name.contains(CORE_SPECIFICATION_TYPE))
                    fdTypeCollections.addAll(fdTypes)
            }
        }
    }
    
    def getDiagnostics() {
    	return diagnostics
    }
    private def validateEnumSizeDeployments() {
        fdTypeCollections.forEach[types.forEach[validateEnumSize(it)]]
        fdInterfaces.forEach [
            types.forEach[validateEnumSize(it)]
            attributes.forEach[validateEnumSize(it)]
            methods.forEach [
                if (inArguments !== null)
                    inArguments.arguments.forEach[validateEnumSize(it)]
                if (outArguments !== null)
                    outArguments.arguments.forEach[validateEnumSize(it)]
            ]
            broadcasts.forEach [
                if (outArguments !== null)
                    outArguments.arguments.forEach[validateEnumSize(it)]
            ]
        ]
        validateCompoundMemberEnumSize
    }

    private def validateCompoundMemberEnumSize() {
        fdTypeCollections.forEach[types.filter(typeof(FDCompound)).forEach[fields.forEach[validateFieldEnumSize(it)]]]
        fdInterfaces.forEach[types.filter(typeof(FDCompound)).forEach[fields.forEach[validateFieldEnumSize(it)]]]

        val overwriteElements = new ArrayList<FDOverwriteElement>
        fdInterfaces.forEach [
            overwriteElements.addAll(attributes.filter[isCompound(target)])
            methods.forEach [
                if (inArguments !== null)
                    overwriteElements.addAll(inArguments.arguments.filter[isCompound(target)])
                if (outArguments !== null)
                    overwriteElements.addAll(outArguments.arguments.filter[isCompound(target)])
            ]
            broadcasts.forEach [
                if (outArguments !== null)
                    overwriteElements.addAll(outArguments.arguments.filter[isCompound(target)])
            ]
        ]
        overwriteElements.forEach [
            if (overwrites instanceof FDCompoundOverwrites)
                (overwrites as FDCompoundOverwrites).fields.forEach[validateFieldEnumSize(it)]
        ]
    }

    private def void validateFieldEnumSize(FDField field) {
        if (isEnum(field.target))
            validateEnumSize(field)

        val overwrites = (field as FDOverwriteElement).overwrites
        if (overwrites instanceof FDCompound) {
            overwrites.fields.forEach [
                validateFieldEnumSize(it)
            ]
        }
    }

    private def validateEnumSize(FDElement fdElem) {
        val enumType = getTargetEnumType(fdElem)
        if (enumType !== null)
            validateEnumSize(fdElem, enumType)
    }

    private def getTargetEnumType(FDElement fdElem) {
        if (fdElem instanceof FDAttribute)
            return getEnum(fdElem.target)
        if (fdElem instanceof FDArgument)
            return getEnum(fdElem.target)
        if (fdElem instanceof FDField)
            return getEnum(fdElem.target)
        if (fdElem instanceof FDEnumeration)
            return getEnum(fdElem.target)
        return null
    }

    private def validateEnumSize(FDElement fdElem, FEnumerationType enumType) {
        var properties = if (fdElem instanceof FDOverwriteElement)
                fdElem.overwrites?.properties
            else
                fdElem.properties

        var FDProperty propBackingType
        if (properties !== null)
            propBackingType = properties.items.findFirst[decl.name == "EnumBackingType"]
        if (propBackingType === null) {
            val fdEnum = getEnumTypeDeployments(enumType)
            if (fdEnum?.properties !== null)
                propBackingType = fdEnum.properties.items.findFirst[decl.name == "EnumBackingType"]
        }

		if ((propBackingType !== null) && !((propBackingType.value.single as FDGeneric).value instanceof FDEnumerator)) {
			// this is a syntax error, caught elsewhere.
			return
		}

        val stringBackingType = if (propBackingType !== null)
                ((propBackingType.value.single as FDGeneric).value as FDEnumerator).name
            else
                "UInt8" // the default, if none is given.

        val BigInteger maxPossibleValue = if (stringBackingType.equals("UInt8"))
                new BigInteger("ff", 16)
            else if (stringBackingType.equals("UInt16"))
                new BigInteger("ffff", 16)
            else if (stringBackingType.equals("UInt32"))
                new BigInteger("ffffffff", 16)
            else if (stringBackingType.equals("UInt64"))
                new BigInteger("ffffffffffffffff", 16)
            else if (stringBackingType.equals("Int8"))
                new BigInteger("7f", 16)
            else if (stringBackingType.equals("Int16"))
                new BigInteger("7fff", 16)
            else if (stringBackingType.equals("Int32"))
                new BigInteger("7fffffff", 16)
            else if (stringBackingType.equals("Int64"))
                new BigInteger("7fffffffffffffff", 16)

        val maxEnumValue = getMaximumEnumerationValue(enumType)
        if (maxEnumValue > maxPossibleValue) {
            if (propBackingType !== null) {
                var diag = new FeatureBasedDiagnostic(Diagnostic.ERROR,
                    "Backing type \"" + stringBackingType + "\" of enumeration type \"" + enumType.name + "\" too small to hold max. enumerator value of: " +
                        maxEnumValue, propBackingType, null, -1, null, null)
                diagnostics.add(diag)
            } else {
                var diag = new FeatureBasedDiagnostic(Diagnostic.ERROR,
                    "Default backing type \"" + stringBackingType + "\" of enumeration type \"" + enumType.name + "\" too small to hold max. enumerator value of: " +
                        maxEnumValue, enumType, null, -1, null, null)
                diagnostics.add(diag)
            }
        }

    }

    private def getMaximumEnumerationValue(FEnumerationType _enumeration) {
        var maximum = maxEnumValues.get(_enumeration)
        if (maximum === null) {
            maximum = BigInteger.ZERO
            for (literal : _enumeration.enumerators) {
                if (literal.value !== null && literal.value != "") {
                    val String enumValue = FrancaGeneratorExtensions.getEnumeratorValue(literal.value)
                    if (enumValue !== null) {
                        val BigInteger literalValue = new BigInteger(enumValue)
                        if (maximum < literalValue)
                            maximum = literalValue
                    }
                }
            }
            maxEnumValues.put(_enumeration, maximum)
        }
        return maximum
    }

/*
    private def getEnumTypeDeployments(FTypeRef typeRef) {
        val enumType = getEnum(typeRef)
        if (enumType !== null) {
            val typeContainer = typeRef.eContainer
            if (typeContainer instanceof FAttribute || typeContainer instanceof FField) {
                for (fdType : fdTypeCollections) {
                    val fd = fdType.types.filter(typeof(FDEnumeration)).findFirst [
                        getEnum(it.target) == enumType
                    ]
                    if (fd !== null)
                        return fd
                }
            }
        }
    }
*/

    private def getEnumTypeDeployments(FEnumerationType enumType) {
        for (fdType : fdTypeCollections) {
            val fd = fdType.types.filter(typeof(FDEnumeration)).findFirst [
                getEnum(it.target) == enumType
            ]
            if (fd !== null)
                return fd
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // Enum
    // /////////////////////////////////////////////////////////////////////////
    private def boolean isEnum(FTypedElement elm) {
        if (elm !== null)
            return isEnum(elm.type)
        return false
    }

    private def boolean isEnum(FTypeRef typeRef) {
        if (typeRef !== null) {
            if (typeRef.derived !== null)
                return isEnum(typeRef.derived)
        }
        return false
    }

    private def boolean isEnum(FType typ) {
        if (typ instanceof FTypeDef) {
            if (typ.actualType !== null)
                return isEnum(typ.actualType)
        } else if (typ instanceof FEnumerationType) {
            return true
        }
        return false
    }

    private def FEnumerationType getEnum(FTypedElement elm) {
        if (elm !== null)
            return getEnum(elm.type)
        return null
    }

    private def FEnumerationType getEnum(FTypeRef typeRef) {
        if (typeRef !== null) {
            if (typeRef.derived !== null)
                return getEnum(typeRef.derived)
        }
        return null
    }

    private def FEnumerationType getEnum(FType typ) {
        if (typ instanceof FTypeDef) {
            if (typ.actualType !== null)
                return getEnum(typ.actualType)
        } else if (typ instanceof FEnumerationType) {
            return typ
        }
        return null
    }

    // /////////////////////////////////////////////////////////////////////////
    // Compound
    // /////////////////////////////////////////////////////////////////////////
    private def boolean isCompound(FTypedElement elm) {
        if (elm !== null)
            return isCompound(elm.type)
        return false
    }

    private def boolean isCompound(FTypeRef typeRef) {
        if (typeRef !== null) {
            if (typeRef.derived !== null)
                return isCompound(typeRef.derived)
        }
        return false
    }

    private def boolean isCompound(FType typ) {
        if (typ instanceof FTypeDef) {
            if (typ.actualType !== null)
                return isCompound(typ.actualType)
        } else if (typ instanceof FCompoundType) {
            return true
        }
        return false
    }
}
