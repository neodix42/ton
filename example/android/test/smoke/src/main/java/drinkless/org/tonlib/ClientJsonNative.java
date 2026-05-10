package drinkless.org.tonlib;

public final class ClientJsonNative {
    static {
        System.loadLibrary("native-lib");
    }

    public native long create();

    public native void destroy(long client);
}
