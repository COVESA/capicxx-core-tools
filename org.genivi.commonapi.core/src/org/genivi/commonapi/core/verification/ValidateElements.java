package org.genivi.commonapi.core.verification;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.franca.core.franca.FArgument;
import org.franca.core.franca.FAttribute;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FEnumerator;
import org.franca.core.franca.FField;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMethod;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FType;
import org.franca.core.franca.FTypeCollection;
import org.franca.deploymodel.dsl.fDeploy.FDArgument;
import org.franca.deploymodel.dsl.fDeploy.FDAttribute;
import org.franca.deploymodel.dsl.fDeploy.FDEnumeration;
import org.franca.deploymodel.dsl.fDeploy.FDField;
import org.franca.deploymodel.dsl.fDeploy.FDInterface;
import org.franca.deploymodel.dsl.fDeploy.FDMethod;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.franca.deploymodel.dsl.fDeploy.FDRootElement;
import org.franca.deploymodel.dsl.fDeploy.FDStruct;
import org.franca.deploymodel.dsl.fDeploy.FDTypeDefinition;
import org.franca.deploymodel.dsl.fDeploy.FDTypes;
import org.genivi.commonapi.core.verification.CppKeywords;

public class ValidateElements {

	private CppKeywords cppKeywords = new CppKeywords();
	private List <Object> listOfElements = new ArrayList<Object>();
    private HashMap<String, Object> inArguments = new HashMap<String, Object>();
	private HashMap<String, Object> inputArguments = new HashMap<String, Object>();
	private List <Object> listOfOutArguments = new ArrayList<Object>();
	private List <FEnumerator> listOfEnumerators = new ArrayList<FEnumerator>();

	/**
	 * Final verification of Attributes and In and Out arguments name when the list is completed
	 * Final verification of Struct Elements and Enumerators name when the list is completed
	 * Final verification of Output argument and Enumerator name after verify the reserved identifiers
	 * Input is a FModel
	 */
	public void validateFModelElements(FModel fModel) {
		for (FInterface fInterface : fModel.getInterfaces()) {
			validateInterfaceAndElementsName(fInterface);
		}
		for (FTypeCollection fTypeCollection : fModel.getTypeCollections()) {
			validateTypecollectionAndElementsName(fTypeCollection);
		}
		// Verify if elements name is a reserved identifier. In that case it will be added a suffix
		if (!listOfElements.isEmpty()) {
			for (Object obj : listOfElements) {
				verifyReservedKeyword(obj);
			}
		}
		// Add a suffix to the Out argument since it has the same name of the input parameter
		// Out arguments can only be verified after check if their name is a reserved identifier
		// In case of In and Out being equal and have as name a reserved identifier the final result will be the name of Out argument with two additional "_" and the In argument with only one additional "_"
		if (!listOfOutArguments.isEmpty()) {
			addSuffixForEqualInOut();
		}
		// To all Enumerators in the list it will be added a suffix
		if (!listOfEnumerators.isEmpty()) {
			verifyEnumerationAndParameterName();
		}
		listOfElements.clear();
		listOfOutArguments.clear();
		listOfEnumerators.clear();
	}

	/**
	 * Final verification of Attributes and In and Out arguments name when the list is completed
	 * Final verification of Struct Elements and Enumerators name when the list is completed
	 * Final verification of Output argument and Enumerator name after verify the reserved identifiers
	 * Input is a FDModel
	 */
	public void validateFDModelElements(FDModel fdModel) {
		for (FDRootElement depl : fdModel.getDeployments()) {
			if (depl instanceof FDInterface) {
				validateInterfaceAndElementsName((FDInterface)depl);
			}
			if (depl instanceof FDTypes) {
				validateTypecollectionAndElementsName((FDTypes)depl);
			}
		}
		// Verify if elements name is a reserved identifier. In that case it will be added a suffix
		if (!listOfElements.isEmpty()) {
			for (Object obj : listOfElements) {
				verifyReservedKeyword(obj);
			}
		}
		// Add a suffix to the Out argument since it has the same name of the input parameter
		// Out arguments can only be verified after check if their name is a reserved identifier
		// In case of In and Out being equal and have as name a reserved identifier the final result will be the name of Out argument with two additional "_" and the In argument with only one additional "_"
		if (!listOfOutArguments.isEmpty()) {
			// listOfOutArguments analyzed before add the suffix in order to remove the arguments that correspond to the FDArgument.target()
			// Otherwise the FArgument would be changed twice - one throught the FDArgument.target() and other throught the FArgument that was previous added to the list
			for (Object outArgument : listOfOutArguments) {
				if (outArgument instanceof FDArgument) {
					FArgument argumentTarget = ((FDArgument)outArgument).getTarget();
					if (listOfOutArguments.contains(argumentTarget)) {
						listOfOutArguments.remove(argumentTarget);
					}
				}
			}
			addSuffixForEqualInOut();
		}
		// To all Enumerators in the list it will be added a suffix
		if (!listOfEnumerators.isEmpty()) {
			verifyEnumerationAndParameterName();
		}
		listOfElements.clear();
		listOfOutArguments.clear();
		listOfEnumerators.clear();
	}

	/**
	 * Add Attributes and In and Out arguments to a list
	 * In case In and Out arguments have the same name the out argument is added to a list
	 * Input: FInterface
	 */
	private void validateInterfaceAndElementsName(FInterface fInterface) {
		for (FMethod fMethod : fInterface.getMethods()) {
			for (FArgument in : fMethod.getInArgs()) {
				if (!listOfElements.contains(in)) {
					listOfElements.add(in);
				}
				inputArguments.put(in.getName(), in);
			}
			for (FArgument out : fMethod.getOutArgs()) {
				if (!listOfElements.contains(out)) {
					listOfElements.add(out);
				}
				// Verify if In and Out arguments have the same name. In that case add Out argument to the list
				String outputKeyName = out.getName();
				if (inputArguments.containsKey(outputKeyName)) {
					if (!listOfOutArguments.contains(out)) {
						listOfOutArguments.add(out);
					}
				}
			}
			inputArguments.clear();
		}

		for (FAttribute fAttribute : fInterface.getAttributes()) {
			if (!listOfElements.contains(fAttribute)) {
				listOfElements.add(fAttribute);
			}
		}
	}

	/**
	 * Add Attributes and In and Out arguments to a list
	 * In case In and Out parameters have the same name the out argument is added to a list
	 * Input: FDInterface
	 */
	private void validateInterfaceAndElementsName(FDInterface fdInterface) {
		for (FDMethod fdMethod : fdInterface.getMethods()) {
			if (fdMethod.getInArguments() != null) {
				for (FDArgument in : fdMethod.getInArguments().getArguments()) {
					if (!listOfElements.contains(in)) {
						listOfElements.add(in);
					}
					inputArguments.put(in.getTarget().getName(), in.getTarget());
				}
			}
			if (fdMethod.getOutArguments() != null) {
				for (FDArgument out : fdMethod.getOutArguments().getArguments()) {
					if (!listOfElements.contains(out)) {
						listOfElements.add(out);
					}
					// Verify if In and Out arguments have the same name. In that case add Out argument to the list
					String outputKeyName = out.getTarget().getName();
					if (inputArguments.containsKey(outputKeyName)) {
						if (!listOfOutArguments.contains(out)) {
							listOfOutArguments.add(out);
						}
					}
				}
			}
			inputArguments.clear();
		}

		for (FDAttribute fdAttribute : fdInterface.getAttributes()) {
			if (!listOfElements.contains(fdAttribute)) {
				listOfElements.add(fdAttribute);
			}
		}

		// Add to the list the elements that are only identified in FModel
		for (FMethod fMethod : fdInterface.getTarget().getMethods()) {
			for (FArgument in : fMethod.getInArgs()) {
				if (!listOfElements.contains(in)) {
					listOfElements.add(in);
				}
				inputArguments.put(in.getName(), in);
			}
			for (FArgument out : fMethod.getOutArgs()) {
				if (!listOfElements.contains(out)) {
					listOfElements.add(out);
				}
				// Verify if In and Out arguments have the same name. In that case add Out argument to the list
				String outputKeyName = out.getName();
				if (inputArguments.containsKey(outputKeyName)) {
					if (!listOfOutArguments.contains(out)) {
						listOfOutArguments.add(out);
					}
				}
			}
			inputArguments.clear();
		}

		for (FAttribute fAttribute : fdInterface.getTarget().getAttributes()) {
			if (!listOfElements.contains(fAttribute)) {
				listOfElements.add(fAttribute);
			}
		}
	}

	/**
	 * Add Enumerators and Struct members to a list
	 * Verify if an Enumeration and Enumerator have the same name. In that case the Enumerator is added to a list for it can be updated later
	 * Input: FTypeCollection
	 */
	private void validateTypecollectionAndElementsName(FTypeCollection fTypeCollection) {
		for (FType fType : fTypeCollection.getTypes()) {
			if (fType instanceof FStructType) {
				for (FField structElements : ((FStructType)fType).getElements()) {
					if (!listOfElements.contains(structElements)) {
						listOfElements.add(structElements);
					}
				}
			}
			if (fType instanceof FEnumerationType) {
				FEnumerationType fEnumerationType = (FEnumerationType)fType;
				for (FEnumerator fEnumerator : fEnumerationType.getEnumerators()) {
					if (!listOfElements.contains(fEnumerator)) {
						listOfElements.add(fEnumerator);
					}
					// Verify if Enumerator and Enumeration have the same name. In that case add Enumerator to the list
					String fEnumeratorName = fEnumerator.getName();
					if (fEnumeratorName.equals(fEnumerationType.getName())) {
						if (!listOfEnumerators.contains(fEnumerator)) {
							listOfEnumerators.add(fEnumerator);
						}
					}
				}
			}
		}
	}

	/**
	 * Add Enumerators and Struct members to a list
	 * Verify if an Enumeration and Enumerator have the same name. In that case the Enumerator is added to a list for it can be updated later
	 * Input: FDTypes
	 */
	private void validateTypecollectionAndElementsName(FDTypes fdTypeCollection) {
		for (FDTypeDefinition fdType : fdTypeCollection.getTypes()) {
			if (fdType instanceof FDStruct) {
				for (FDField structElements : ((FDStruct)fdType).getFields()) {
					if (!listOfElements.contains(structElements)) {
						listOfElements.add(structElements);
					}
				}
			}
			if (fdType instanceof FDEnumeration) {
				FEnumerationType fEnumerationType = ((FDEnumeration)fdType).getTarget();
				for (FEnumerator fEnumerator : fEnumerationType.getEnumerators()) {
					if (!listOfElements.contains(fEnumerator)) {
						listOfElements.add(fEnumerator);
					}
					// Verify if Enumerator and Enumeration have the same name. In that case add Enumerator to the list
					String fEnumeratorName = fEnumerator.getName();
					if (fEnumeratorName.equals(fEnumerationType.getName())) {
						if (!listOfEnumerators.contains(fEnumerator)) {
							listOfEnumerators.add(fEnumerator);
						}
					}
				}
			}
		}

		// Add to the list the elements that are only identified in FModel
		for (FType fType : fdTypeCollection.getTarget().getTypes()) {
			if (fType instanceof FStructType) {
				for (FField structElements : ((FStructType)fType).getElements()) {
					if (!listOfElements.contains(structElements)) {
						listOfElements.add(structElements);
					}
				}
			}
			if (fType instanceof FEnumerationType) {
				FEnumerationType fEnumerationType = (FEnumerationType)fType;
				for (FEnumerator fEnumerator : fEnumerationType.getEnumerators()) {
					if (!listOfElements.contains(fEnumerator)) {
						listOfElements.add(fEnumerator);
					}
					// Verify if Enumerator and Enumeration have the same name. In that case add Enumerator to the list
					String fEnumeratorName = fEnumerator.getName();
					if (fEnumeratorName.equals(fEnumerationType.getName())) {
						if (!listOfEnumerators.contains(fEnumerator)) {
							listOfEnumerators.add(fEnumerator);
						}
					}
				}
			}
		}
	}

	/**
	 * Add a suffix "_" to the elements it receives as input 
	 * Method used when its already verified that In and Out arguments have the same name
	 */
	private void addSuffixForEqualInOut() {
		for (Object object : listOfOutArguments) {
			if (object instanceof FArgument) {
				String outputKeyName = ((FArgument)object).getName();
				String outputKeyWithSuffix = outputKeyName + "_";
				((FArgument)object).setName(outputKeyWithSuffix);
			}
			else if (object instanceof FDArgument) {
				String outputKeyName = (((FDArgument)object).getTarget()).getName();
				String outputKeyWithSuffix = outputKeyName + "_";
				(((FDArgument)object).getTarget()).setName(outputKeyWithSuffix);
			}
		}
	}

	/**
	 * Verify if In and Out parameters have the same name
	 * In that case add a suffix "_" to the Out element
	 */
	public void verifyEqualInOutAndAddSuffix(Object object) {
		if (object instanceof FModel) {
			for (FInterface fInterface : ((FModel)object).getInterfaces()) {
				for (FMethod fMethod : fInterface.getMethods()) {
					for (FArgument in : fMethod.getInArgs()) {
						inArguments.put(in.getName(), in);
					}
					for (FArgument out : fMethod.getOutArgs()) {
						String outputKeyName = out.getName();
						if (inArguments.containsKey(outputKeyName)) {
							String outputKeyWithSuffix = outputKeyName + "_";
							out.setName(outputKeyWithSuffix);
						}
					}
					inArguments.clear();
				}
			}
		}
		else if (object instanceof FDModel) {
			for (FDRootElement depl : ((FDModel)object).getDeployments()) {
				if (depl instanceof FDInterface) {
					for (FDMethod fdMethod : ((FDInterface)depl).getMethods()) {
						if (fdMethod.getInArguments() != null) {
							for (FDArgument in : fdMethod.getInArguments().getArguments()) {
								inArguments.put(in.getTarget().getName(), in.getTarget());
							}
						}
						if (fdMethod.getOutArguments() != null) {
							for (FDArgument out : fdMethod.getOutArguments().getArguments()) {
								String outputKeyName = (out.getTarget()).getName();
								if (inArguments.containsKey(outputKeyName)) {
									String outputKeyWithSuffix = outputKeyName + "_";
									(out.getTarget()).setName(outputKeyWithSuffix);
								}
							}
						}
						inArguments.clear();
					}
					for (FMethod fMethod : ((FDInterface)depl).getTarget().getMethods()) {
						for (FArgument in : fMethod.getInArgs()) {
							inArguments.put(in.getName(), in);
						}
						for (FArgument out : fMethod.getOutArgs()) {
							String outputKeyName = out.getName();
							if (inArguments.containsKey(outputKeyName)) {
								String outputKeyWithSuffix = outputKeyName + "_";
								out.setName(outputKeyWithSuffix);
							}
						}
						inArguments.clear();
					}
				}
			}
		}
	}

	/**
	 * Add a suffix "_" to the elements it receives as input
	 * Method used when its already verified that Enumerator has the same name of Enumeration
	 */
	private void verifyEnumerationAndParameterName() {
		for (FEnumerator fEnumerator : listOfEnumerators) {
			String fEnumeratorName = fEnumerator.getName();
			String fEnumeratorNameWithSuffix = fEnumeratorName + "_";
			fEnumerator.setName(fEnumeratorNameWithSuffix);
		}
	}

	/**
	 * Verify if the input name is a reserved identifier. In that case add a suffix "_"
	 */
	public void verifyReservedKeyword(Object object) {
		if (object instanceof FArgument) {
			String reservedKeyword = ((FArgument)object).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				((FArgument)object).setName(reservedKeywordWithSuffix);
			}
		}
 		else if (object instanceof FDArgument) {
			String reservedKeyword = (((FDArgument)object).getTarget()).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				(((FDArgument)object).getTarget()).setName(reservedKeywordWithSuffix);
			}
		}
 		else if (object instanceof FField) {
			String reservedKeyword = ((FField)object).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				((FField)object).setName(reservedKeywordWithSuffix);
			}
		}
 		else if (object instanceof FDField) {
			String reservedKeyword = (((FDField)object).getTarget()).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				(((FDField)object).getTarget()).setName(reservedKeywordWithSuffix);
			}
		}
		else if (object instanceof FAttribute) {
			String reservedKeyword = ((FAttribute)object).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				((FAttribute)object).setName(reservedKeywordWithSuffix);
			}
		}
 		else if (object instanceof FDAttribute) {
			String reservedKeyword = (((FDAttribute)object).getTarget()).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				(((FDAttribute)object).getTarget()).setName(reservedKeywordWithSuffix);
			}
		}
 		else if (object instanceof FEnumerator) {
			String reservedKeyword = ((FEnumerator)object).getName();
			if (cppKeywords.handleReservedWords.containsKey(reservedKeyword)) {
				String reservedKeywordWithSuffix = cppKeywords.handleReservedWords.get(reservedKeyword);
				((FEnumerator)object).setName(reservedKeywordWithSuffix);
			}
		}
	}
}