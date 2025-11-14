float TraceShadowRay(RaytracingAccelerationStructure accelStruct, float3 origin, float3 direction, float maxDist, RAY_FLAG flags)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    flags |= RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    query.TraceRayInline(accelStruct, flags, 0xFF, ray);
    query.Proceed();

    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
}
