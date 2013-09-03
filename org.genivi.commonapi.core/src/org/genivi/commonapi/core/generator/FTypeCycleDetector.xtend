package org.genivi.commonapi.core.generator

import java.util.HashMap
import java.util.List
import java.util.Stack
import org.franca.core.franca.FType
import javax.inject.Inject

class FTypeCycleDetector {
    @Inject
    private extension FrancaGeneratorExtensions francaGeneratorExtensions

    private val indices = new HashMap<FType, Integer>
    private val lowlink = new HashMap<FType, Integer>
    private val stack = new Stack<FType>
    private var int index
    public var String outErrorString

    new(FrancaGeneratorExtensions francaGeneratorExtensions) {
        this.francaGeneratorExtensions = francaGeneratorExtensions
    }

    new() {
    }

    def dispatch hasCycle(FType type) {
        indices.clear()
        lowlink.clear()
        stack.clear()
        index = 0
        outErrorString = type.name + "->";
        return tarjan(type)
    }

    def dispatch hasCycle(List<FType> types) {
        indices.clear()
        lowlink.clear()
        stack.clear()
        index = 0

        val typeWithCycle = types.findFirst[type|!indices.containsKey(type) && tarjan(type)]

        return typeWithCycle != null
    }

    // Tarjan's Strongly Connected Components Algorithm
    // returns true if a cycle was detected
    /**
     * Tarjan's Strongly Connected Components Algorithm
     *
     * @param type
     *            start searching from type.
     * @return <code>true</code> if a dependency cycle was detected.

     */
    def private boolean tarjan(FType type) {
        indices.put(type, index)
        lowlink.put(type, index)
        index = index + 1

        stack.push(type)

        val directlyReferencedTypes = type.directlyReferencedTypes

        for (referencedType : directlyReferencedTypes) {
            outErrorString = outErrorString + referencedType.name + "->"
            if (!indices.containsKey(referencedType)) {
                if (tarjan(referencedType))
                    return true

                lowlink.put(
                    type,
                    Math::min(lowlink.get(type), lowlink.get(referencedType))
                );
            } else if (stack.contains(referencedType))
                lowlink.put(
                    type,
                    Math::min(lowlink.get(type), indices.get(referencedType))
                );
        }

        // if scc root and not on top of stack, then we have a cycle (scc size > 1)
        if (lowlink.get(type) == indices.get(type) && !stack.pop().equals(type)) {
            outErrorString = outErrorString.subSequence(0, outErrorString.length - 2) as String
            return true;
        }

        return false
    }
}
