package drinkless.org.ton;

import android.support.test.runner.AndroidJUnit4;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class TonSmokeLoadTest {

    @Test
    public void execute_smoke() {
        TonApi.Object result = Client.execute(new TonApi.GetLogVerbosityLevel());
        assertNotNull(result);
    }

    @Test
    public void create_destroy_native_client_smoke() throws Exception {
        Method createNativeClient = Client.class.getDeclaredMethod("createNativeClient");
        Method destroyNativeClient = Client.class.getDeclaredMethod("destroyNativeClient", long.class);
        createNativeClient.setAccessible(true);
        destroyNativeClient.setAccessible(true);

        long clientId = (Long) createNativeClient.invoke(null);
        assertTrue(clientId != 0L);
        destroyNativeClient.invoke(null, clientId);
    }
}
