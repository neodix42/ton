package drinkless.org.tonlib;

import android.support.test.runner.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class JavaSmokeLoadTest {
    @Test
    public void create_destroy_client_smoke() {
        ClientJsonNative nativeApi = new ClientJsonNative();
        long client = nativeApi.create();
        assertTrue(client != 0L);
        nativeApi.set_verbosity_level(0);
        nativeApi.destroy(client);
    }
}
