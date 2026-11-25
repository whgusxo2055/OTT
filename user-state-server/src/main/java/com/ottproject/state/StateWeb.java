package com.ottproject.state;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.HandlerInterceptor;
import org.springframework.web.servlet.config.annotation.*;

@Configuration
class StateWeb implements WebMvcConfigurer {
    private final MemberResolver members;
    private final String[] origins;

    StateWeb(MemberResolver members, @Value("${STATE_ALLOWED_ORIGINS:http://localhost:8080,http://127.0.0.1:8080}") String[] origins) {
        this.members = members;
        this.origins = origins;
    }

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(new HandlerInterceptor() {
            @Override
            public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
                if (!request.getMethod().equals("OPTIONS")) {
                    response.setHeader("Cache-Control", "no-store");
                    request.setAttribute("member", members.resolve(request.getHeader("Authorization")));
                }
                return true;
            }
        }).addPathPatterns("/api/v1/me/**");
    }

    @Override
    public void addCorsMappings(CorsRegistry registry) {
        registry.addMapping("/api/v1/me/**").allowedOrigins(origins)
                .allowedMethods("GET", "PUT", "DELETE").allowedHeaders("Authorization", "Content-Type");
    }
}
