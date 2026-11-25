package com.ottproject.state;

import java.util.UUID;

interface MemberResolver {
    UUID resolve(String authorization);
}
