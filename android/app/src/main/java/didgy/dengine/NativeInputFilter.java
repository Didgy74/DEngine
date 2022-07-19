package didgy.dengine;

import android.text.InputType;

/*
    This enum HAS TO MATCH
    the one in C++ (DEngine::Application::SoftInputFilter)
 */
enum NativeInputFilter {
    TEXT (0),
    INTEGER (1),
    UNSIGNED_INTEGER (2),
    FLOAT (3),
    UNSIGNED_FLOAT (4);

    int value;
    NativeInputFilter(int nativeInt) {
        this.value = nativeInt;
    }

    public boolean isNumpadInput() {
        switch (this) {
            case INTEGER:
            case UNSIGNED_INTEGER:
            case FLOAT:
            case UNSIGNED_FLOAT:
                return true;
        }
        return false;
    }

    public int toAndroidInputType() {
        switch (this) {
            case TEXT:
                return
                    InputType.TYPE_CLASS_TEXT |
                    InputType.TYPE_TEXT_FLAG_AUTO_CORRECT |
                    InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            case INTEGER:
                return InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;
            case UNSIGNED_INTEGER:
                return InputType.TYPE_CLASS_NUMBER;
            case FLOAT:
                return
                    InputType.TYPE_CLASS_NUMBER |
                    InputType.TYPE_NUMBER_FLAG_DECIMAL |
                    InputType.TYPE_NUMBER_FLAG_SIGNED;
            case UNSIGNED_FLOAT:
                return InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL;
        }
        assert(false);
        return 0;
    }

    // Input should be the integer we received from the native callbacks.
    static public NativeInputFilter fromNativeEnum(int nativeFilter) {
        for (var item : NativeInputFilter.values()) {
            if (item.value == nativeFilter)
                return item;
        }
        assert(false);
        return null;
    }

    interface FilterFn {
        boolean isValidChar(
                int replaceStart,
                int replaceCount,
                char item,
                int itemPos);
    }
    static class DotFilter implements FilterFn {
        int dotsLeft = 0;
        DotFilter(String currentText) {
            if (!currentText.contains("."))
                dotsLeft = 1;
        }
        @Override
        public boolean isValidChar(
                int replaceStart,
                int replaceCount,
                char item,
                int itemPos)
        {
            if (item == '.') {
                if (dotsLeft > 0) {
                    dotsLeft -= 1;
                    return true;
                }

                return false;
            }

            return true;
        }
    }

    private static boolean isNumber(char unicode) {
        return unicode >= '0' && unicode <= '9';
    }
    private static boolean isFloatSymbol(char unicode) {
        return isNumber(unicode) || unicode == '-' || unicode == '+' || unicode == '.';
    }

    static boolean isValidChar_isNumber(
            int replaceStart,
            int replaceCount,
            char item,
            int itemPos) {
        return isNumber(item);
    }
    static boolean isValidChar_isFloatSymbol(
            int replaceStart,
            int replaceCount,
            char item,
            int itemPos) {
        return isFloatSymbol(item);
    }
    static boolean isValidChar_NumberSignFilter(
            int replaceStart,
            int replaceCount,
            char item,
            int itemPos) {
        // Number signs has to be the first element
        if (item == '+' || item == '-') {
            return replaceStart == 0 && itemPos == 0;
        }
        return true;
    }

    private FilterFn[] buildFilterList(String currentText) {
        switch (this) {
            case TEXT:
                return new FilterFn[]{};
            case INTEGER:
                return new FilterFn[]{ NativeInputFilter::isValidChar_isNumber };
            case FLOAT:
                return new FilterFn[]{
                        NativeInputFilter::isValidChar_isFloatSymbol,
                        NativeInputFilter::isValidChar_NumberSignFilter,
                        new DotFilter(currentText) };
        }
        assert false;
        return null;
    }
    public CharSequence filterText(
            String currentText,
            int replaceStart,
            int replaceCount,
            CharSequence input)
    {
        var filters = buildFilterList(currentText);
        assert filters != null;

        var outString = new StringBuilder();

        for (var i = 0; i < input.length(); i++) {
            var item = input.charAt(i);

            var outStringLen = outString.length();

            var valid = true;
            for (var filter : filters) {
                var filterResult = filter.isValidChar(
                        replaceStart,
                        replaceCount,
                        item,
                        outStringLen);
                if (!filterResult) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                outString.append(item);
        }

        return outString;
    }
}

