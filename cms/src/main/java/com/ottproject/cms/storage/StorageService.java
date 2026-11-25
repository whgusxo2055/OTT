package com.ottproject.cms.storage;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import software.amazon.awssdk.services.s3.presigner.S3Presigner;
import software.amazon.awssdk.services.s3.presigner.model.GetObjectPresignRequest;

/** 브라우저가 포스터를 받을 수 있는 서명 주소를 만든다. */
@Service
public class StorageService {

    private final S3Presigner presigner;
    private final String bucket;
    private final Duration urlTtl;

    public StorageService(S3Presigner presigner,
                          @Value("${storage.bucket}") String bucket,
                          @Value("${storage.poster-url-ttl-minutes}") long urlTtlMinutes) {
        this.presigner = presigner;
        this.bucket = bucket;
        this.urlTtl = Duration.ofMinutes(urlTtlMinutes);
    }

    /** 유효 시간이 지나면 못 쓰는 임시 주소. 버킷은 비공개로 두고 이 주소로만 받게 한다. */
    public String presignedUrl(String key) {
        GetObjectPresignRequest request = GetObjectPresignRequest.builder()
                .signatureDuration(urlTtl)
                .getObjectRequest(get -> get.bucket(bucket).key(key))
                .build();
        return presigner.presignGetObject(request).url().toString();
    }
}
