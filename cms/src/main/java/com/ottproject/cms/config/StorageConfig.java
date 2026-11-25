package com.ottproject.cms.config;

import java.net.URI;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import software.amazon.awssdk.auth.credentials.AwsBasicCredentials;
import software.amazon.awssdk.auth.credentials.StaticCredentialsProvider;
import software.amazon.awssdk.regions.Region;
import software.amazon.awssdk.services.s3.S3Configuration;
import software.amazon.awssdk.services.s3.presigner.S3Presigner;

/**
 * NCP Object Storage 연결 설정.
 * AWS S3와 같은 API(S3 호환)를 쓰므로 엔드포인트만 NCP 주소로 바꿔서 AWS SDK를 그대로 사용한다.
 */
@Configuration
public class StorageConfig {

    private final URI endpoint;
    private final Region region;
    private final StaticCredentialsProvider credentials;
    // 주소 형식을 https://엔드포인트/버킷/키 로 고정 (path-style)
    private final S3Configuration pathStyle = S3Configuration.builder().pathStyleAccessEnabled(true).build();

    public StorageConfig(@Value("${storage.endpoint}") String endpoint,
                         @Value("${storage.region}") String region,
                         @Value("${storage.access-key}") String accessKey,
                         @Value("${storage.secret-key}") String secretKey) {
        this.endpoint = URI.create(endpoint);
        this.region = Region.of(region);
        this.credentials = StaticCredentialsProvider.create(AwsBasicCredentials.create(accessKey, secretKey));
    }

    /** 유효 시간이 있는 서명 주소(pre-signed URL)를 만들 때 쓰는 도구 */
    @Bean
    public S3Presigner s3Presigner() {
        return S3Presigner.builder()
                .endpointOverride(endpoint)
                .region(region)
                .credentialsProvider(credentials)
                .serviceConfiguration(pathStyle)
                .build();
    }
}
