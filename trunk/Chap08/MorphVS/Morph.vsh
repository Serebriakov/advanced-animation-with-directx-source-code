; v0 = Mesh 1 position.xyz
; v1 = Mesh 1 normal.xyz
; v2 = Mesh 1 Texture.xy
; v3 = Mesh 2 position.xyz
; v4 = Mesh 2 normal.xyz
;
; c0-c3   = world+view+projection matrix
; c4      = morph amount (0-1, 1.0f- 0-1, 0, 0)
; c5      = light direction
vs.1.0

; declare mapping
dcl_position   v0
dcl_normal     v1
dcl_texcoord   v2
dcl_position1  v3
dcl_normal1    v4

; lerp between positions using c4.x/y scalars
mul r0, v0, c4.x
mad r0, v3, c4.y, r0

; lerp between normals using c4.x/y scalars
mul r1, v1, c4.x
mad r1, v4, c4.y, r1

; Project position
m4x4 oPos, r0, c0

; Dot normal with inversed light direction 
; to get diffuse color
dp3 oD0, r1, -c5

; Store texture coordinates
mov oT0.xy, v2
