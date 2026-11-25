package com.ottproject.cms.config;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.CorsRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

/**
 * 목업을 다른 주소(예: 내 컴퓨터의 index.html 파일)에서 열어도 API를 부를 수 있게 허용한다.
 * 지금은 조회(GET)만 있으므로 모든 출처를 허용한다. 로그인·관리 기능을 붙일 때 범위를 좁혀야 한다.
 */
@Configuration
public class WebConfig implements WebMvcConfigurer {

    @Override
    public void addCorsMappings(CorsRegistry registry) {
        registry.addMapping("/api/**")
                .allowedOrigins("*")
                .allowedMethods("GET", "HEAD", "OPTIONS");
    }
}
