package didgy.dengine

class BeginInputSessionInfo(
    val activity: DEngineActivity,
    val inputFilter: NativeInputFilter,
    val text: String,
    val selStart: Int,
    val selCount: Int) {
    val selEnd: Int
        get() = selStart + selCount
}