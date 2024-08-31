package ru.yandex.alice.paskills.common.apphost.spring;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import io.grpc.ForwardingServerCallListener;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.web.apphost.grpc.proto.TServiceRequest;

public class AmmoGenServerInterceptor implements ServerInterceptor {

    private static final SimpleDateFormat REQUEST_AMMO_DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd_HH:mm:ss:SSS");
    private static final SimpleDateFormat ALL_REQUESTS_AMMO_DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd_HH:mm");
    private static final String AMMOS_DIR = "ammos";

    private static final Logger logger = LogManager.getLogger();

    private final ObjectMapper objectMapper;
    private final String allRequestsAmmosFilename;

    public AmmoGenServerInterceptor(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
        this.allRequestsAmmosFilename = String.format("%s/%s_all_requests_ammos.ammo",
                AMMOS_DIR,
                ALL_REQUESTS_AMMO_DATE_FORMAT.format(new Date()));
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(ServerCall<ReqT, RespT> call, Metadata headers,
                                                                 ServerCallHandler<ReqT, RespT> next) {
        final ServerCall.Listener<ReqT> original = next.startCall(call, headers);
        return new ForwardingServerCallListener.SimpleForwardingServerCallListener<>(original) {
            @Override
            public void onMessage(ReqT message) {
                super.onMessage(message);
                genAmmoFile(call, message);
            }
        };
    }

    private <ReqT, RespT> void genAmmoFile(ServerCall<ReqT, RespT> call, ReqT message) {
        if (message instanceof TServiceRequest) {
            String requestPath = ((TServiceRequest) message).getPath();
            InputStream requestInput = call.getMethodDescriptor().streamRequest(message);
            String requestAmmoFilename = String.format("%s/%s_%s.ammo",
                    AMMOS_DIR,
                    REQUEST_AMMO_DATE_FORMAT.format(new Date()),
                    requestPath.replaceAll("[-/]", "_").replaceFirst("^_", ""));
            File requestAmmoTargetFile = new File(requestAmmoFilename);
            try {
                Files.createDirectories(Paths.get(AMMOS_DIR));
                byte[] encodedTServiceRequest = requestInput.readAllBytes();
                requestInput.close();

                AmmoData requestAmmoData = new AmmoData(encodedTServiceRequest);
                byte[] requestAmmoBytes = objectMapper.writeValueAsString(requestAmmoData)
                        .getBytes(StandardCharsets.UTF_8);
                FileOutputStream fos = new FileOutputStream(requestAmmoTargetFile);
                fos.write(requestAmmoBytes);
                fos.close();

                FileOutputStream allRequestsFw = new FileOutputStream(allRequestsAmmosFilename, true);
                allRequestsFw.write(requestAmmoBytes);
                allRequestsFw.write("\n".getBytes(StandardCharsets.UTF_8));
                allRequestsFw.close();
            } catch (IOException e) {
                logger.warn("Generating request ammo file failed", e);
            }
        }
    }

    private static class AmmoData {
        @JsonProperty("Data")
        private byte[] data;

        AmmoData(byte[] data) {
            this.data = data;
        }
    }
}
