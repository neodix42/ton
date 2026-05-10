package drinkless.org.tonlib;

public final class ClientJsonNative {
    static {
        System.loadLibrary("native-lib");
    }

    public native void set_verbosity_level(int verbosity_level);

    public native long create();

    public native void destroy(long client);
}
