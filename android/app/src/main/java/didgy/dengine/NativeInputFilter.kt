package didgy.dengine

import android.text.InputType


typealias IsValidCharFn = (
    replaceStart: Int,
    replaceCount: Int,
    item: Char,
    itemPos: Int) -> Boolean

/*
   This enum HAS TO MATCH
   the one in C++ (DEngine::Application::SoftInputFilter)
*/
enum class NativeInputFilter(var value: Int) {
    TEXT(0),
    INTEGER(1),
    UNSIGNED_INTEGER(2),
    FLOAT(3),
    UNSIGNED_FLOAT(4);

    val isNumpadInput: Boolean
        get() {
            return when (this) {
                INTEGER, UNSIGNED_INTEGER, FLOAT, UNSIGNED_FLOAT -> true
                else -> false
            }
        }

    fun toAndroidInputType(): Int {
        return when (this) {
            TEXT -> {
                InputType.TYPE_CLASS_TEXT or
                        InputType.TYPE_TEXT_FLAG_AUTO_CORRECT or
                        InputType.TYPE_TEXT_FLAG_CAP_SENTENCES
            }
            INTEGER -> InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_SIGNED
            UNSIGNED_INTEGER -> InputType.TYPE_CLASS_NUMBER
            FLOAT -> {
                InputType.TYPE_CLASS_NUMBER or
                InputType.TYPE_NUMBER_FLAG_DECIMAL or
                InputType.TYPE_NUMBER_FLAG_SIGNED
            }
            UNSIGNED_FLOAT -> {
                InputType.TYPE_CLASS_NUMBER or
                InputType.TYPE_NUMBER_FLAG_DECIMAL
            }
        }
    }

    fun dotFilter(currentText: String): IsValidCharFn {
        var dotsLeft = 0
        if (!currentText.contains("."))
            dotsLeft = 1
        return lambda@{ _, _, item, _ ->
            if (item == '.') {
                if (dotsLeft > 0) {
                    dotsLeft -= 1
                    return@lambda true
                }
                return@lambda false
            }
            return@lambda true
        }
    }

    private fun buildFilterList(currentText: String): List<IsValidCharFn> =
        when (this) {
            INTEGER -> listOf(::isValidChar_isNumber)
            FLOAT -> listOf(
                ::isValidChar_isFloatSymbol,
                ::isValidChar_NumberSignFilter,
                dotFilter(currentText))
            else -> listOf()
        }

    fun filterText(
        currentText: String,
        replaceStart: Int,
        replaceCount: Int,
        input: CharSequence
    ): CharSequence {
        val filters = buildFilterList(currentText)
        return input.fold(StringBuilder()) { accum, char ->
            var valid = true
            for (filter in filters) {
                val filterResult = filter(
                    replaceStart,
                    replaceCount,
                    char,
                    accum.length)
                if (!filterResult) {
                    valid = false
                    break
                }
            }
            if (valid) {
                accum.append(char)
            }
            accum
        }.toString()
    }

    companion object {
        // Input should be the integer we received from the native callbacks.
        fun fromNativeEnum(nativeFilter: Int): NativeInputFilter {
            for (item in values()) {
                if (item.value == nativeFilter) return item
            }
            throw RuntimeException("Tried to create a NativeInputFilter from illegal integer.")
        }

        private fun isNumber(unicode: Char): Boolean {
            return unicode in '0'..'9'
        }

        private fun isFloatSymbol(unicode: Char): Boolean {
            return isNumber(unicode) || unicode == '-' || unicode == '+' || unicode == '.'
        }

        fun isValidChar_isNumber(
            replaceStart: Int,
            replaceCount: Int,
            item: Char,
            itemPos: Int
        ): Boolean {
            return isNumber(item)
        }

        fun isValidChar_isFloatSymbol(
            replaceStart: Int,
            replaceCount: Int,
            item: Char,
            itemPos: Int
        ): Boolean {
            return isFloatSymbol(item)
        }

        fun isValidChar_NumberSignFilter(
            replaceStart: Int,
            replaceCount: Int,
            item: Char,
            itemPos: Int
        ): Boolean {
            // Number signs has to be the first element
            return if (item == '+' || item == '-') {
                replaceStart == 0 && itemPos == 0
            } else true
        }
    }
}