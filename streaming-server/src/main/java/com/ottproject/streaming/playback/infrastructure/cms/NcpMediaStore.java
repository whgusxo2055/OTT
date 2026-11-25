package com.ottproject.streaming.playback.infrastructure.cms;

import com.ottproject.streaming.playback.application.port.MediaStore;
import com.ottproject.streaming.playback.infrastructure.LocalMediaStore;
import jakarta.annotation.PreDestroy;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.Optional;
import java.util.regex.Pattern;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Primary;
import org.springframework.core.io.AbstractResource;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.core.io.Resource;
import org.springframework.stereotype.Component;
import software.amazon.awssdk.auth.credentials.AwsBasicCredentials;
import software.amazon.awssdk.auth.credentials.StaticCredentialsProvider;
import software.amazon.awssdk.core.exception.SdkException;
import software.amazon.awssdk.regions.Region;
import software.amazon.awssdk.services.s3.S3Client;
import software.amazon.awssdk.services.s3.S3Configuration;
import software.amazon.awssdk.services.s3.model.S3Exception;

@Component
@Primary
@ConditionalOnProperty("streaming.cms.base-url")
class NcpMediaStore implements MediaStore {

    private static final int MAX_MANIFEST_BYTES = 1024 * 1024;
    private static final Pattern SEGMENT_NAME = Pattern.compile("[A-Za-z0-9_.-]+\\.ts");
    private static final Pattern URI_ATTRIBUTE = Pattern.compile("(?:[:,])\\s*[A-Z0-9-]*URI\\s*=");

    private final LocalMediaStore local;
    private final S3Client storage;
    private final String bucket;

    NcpMediaStore(LocalMediaStore local,
                  @Value("${NCP_ENDPOINT:https://kr.object.ncloudstorage.com}") URI endpoint,
                  @Value("${NCP_REGION:kr-standard}") String region,
                  @Value("${NCP_BUCKET:acme}") String bucket,
                  @Value("${NCP_ACCESS_KEY}") String accessKey,
                  @Value("${NCP_SECRET_KEY}") String secretKey) {
        this.local = local;
        this.bucket = bucket;
        storage = S3Client.builder()
                .endpointOverride(endpoint)
                .region(Region.of(region))
                .credentialsProvider(StaticCredentialsProvider.create(AwsBasicCredentials.create(accessKey, secretKey)))
                .serviceConfiguration(S3Configuration.builder().pathStyleAccessEnabled(true).build())
                .overrideConfiguration(config -> config.apiCallAttemptTimeout(Duration.ofSeconds(15))
                        .apiCallTimeout(Duration.ofSeconds(25)))
                .build();
    }

    @Override
    public Optional<Resource> find(String key) throws IOException {
        if (!key.startsWith(CmsContentCatalog.CMS_MEDIA_PREFIX)) {
            return local.find(key);
        }

        String objectKey = key.substring(CmsContentCatalog.CMS_MEDIA_PREFIX.length());
        try {
            if (objectKey.endsWith("/index.m3u8")) {
                return Optional.of(new ByteArrayResource(readManifest(objectKey)));
            }
            Long length = storage.headObject(request -> request.bucket(bucket).key(objectKey)).contentLength();
            if (length == null || length < 0) {
                throw new IOException("Object storage did not return the segment length");
            }
            return Optional.of(new ObjectSegment(objectKey, length));
        } catch (S3Exception exception) {
            if (exception.statusCode() == 404) {
                return Optional.empty();
            }
            throw new IOException("Object storage could not read playback media", exception);
        } catch (SdkException exception) {
            throw new IOException("Object storage could not read playback media", exception);
        }
    }

    private byte[] readManifest(String key) throws IOException {
        try (var input = storage.getObject(request -> request.bucket(bucket).key(key))) {
            byte[] bytes = input.readNBytes(MAX_MANIFEST_BYTES + 1);
            if (bytes.length > MAX_MANIFEST_BYTES) {
                input.abort();
                throw new IOException("HLS playlist exceeds the size limit");
            }

            var lines = new String(bytes, StandardCharsets.UTF_8).lines().toList();
            if (lines.isEmpty() || !"#EXTM3U".equals(lines.getFirst().strip())) {
                throw new IOException("Object storage returned an invalid HLS playlist");
            }
            var manifest = new StringBuilder();
            for (String line : lines) {
                String value = line.strip();
                if (!value.isEmpty() && !value.startsWith("#") && !SEGMENT_NAME.matcher(value).matches()) {
                    throw new IOException("HLS playlist contains an unsupported segment reference");
                }
                if (value.startsWith("#EXT-X-") && URI_ATTRIBUTE.matcher(value).find()) {
                    throw new IOException("HLS playlist contains an unsupported URI attribute");
                }
                manifest.append(value).append('\n');
            }
            return manifest.toString().getBytes(StandardCharsets.UTF_8);
        }
    }

    @PreDestroy
    void close() {
        storage.close();
    }

    private final class ObjectSegment extends AbstractResource {

        private final String key;
        private final long length;

        private ObjectSegment(String key, long length) {
            this.key = key;
            this.length = length;
        }

        @Override
        public long contentLength() {
            return length;
        }

        @Override
        public InputStream getInputStream() throws IOException {
            try {
                return storage.getObject(request -> request.bucket(bucket).key(key));
            } catch (SdkException exception) {
                throw new IOException("Object storage could not open a segment for delivery", exception);
            }
        }

        @Override
        public String getDescription() {
            return "Object storage HLS segment";
        }
    }
}
