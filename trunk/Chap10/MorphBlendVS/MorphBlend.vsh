; Blended Morph Vertex Shader (BlendMorph.vsh)
; by Jim Adams, 2002
; Supports four blend channels with one base mesh channel
;
; v0  = Base mesh's position.xyz
; v1  = Base mesh's normal.xyz
; v2  = Base mesh's texture.xy
;
; v3  = Blend 1 mesh's position.xyz
; v4  = Blend 1 mesh's normal.xyz
; v5  = Blend 1 mesh's texture.xy
;
; v6  = Blend 2 mesh's position.xyz
; v7  = Blend 2 mesh's normal.xyz
; v8  = Blend 2 mesh's texture.xy
;
; v9  = Blend 3 mesh's position.xyz
; v10 = Blend 3 mesh's normal.xyz
; v11 = Blend 3 mesh's texture.xy
;
; v12 = Blend 4 mesh's position.xyz
; v13 = Blend 4 mesh's normal.xyz
; v14 = Blend 4 mesh's texture.xy
;
; c0-c3   = world+view+projection matrix
; c4      = Blend amounts 0-1 (mesh1, mesh2, mesh3, mesh4)
; c5      = light direction
vs.1.0

; declare mapping
dcl_position   v0
dcl_normal     v1
dcl_texcoord   v2

dcl_position1  v3
dcl_normal1    v4
dcl_texcoord1  v5

dcl_position2  v6
dcl_normal2    v7
dcl_texcoord2  v8

dcl_position3  v9
dcl_normal3    v10
dcl_texcoord3  v11

dcl_position4  v12
dcl_normal4    v13
dcl_texcoord4  v14

; Get base coordinates and normal into registers r0 and r1
mov r0, v0  ; coordinates (r0)
mov r1, v1  ; normal      (r1)

; Get differences from 1st blended mesh and add into result
sub r2, v3, r0          ; Get difference in coordinates
mad r4, r2, c4.x, r0    ; Put resulting coordinates into r4
sub r3, v4, r1          ; Get difference in normal
mad r5, r3, c4.x, r1    ; Put resulting normal into r5

; Get differences from 2nd blended mesh and add into result
sub r2, v6, r0          ; Get difference in coordinates
mad r4, r2, c4.y, r4    ; Add resulting coordinates to r4
sub r3, v7, r1          ; Get difference in normal
mad r5, r3, c4.y, r5    ; Add resulting normal to r5

; Get differences from 3rd blended mesh and add into result
sub r2, v9,  r0         ; Get difference in coordinates
mad r4, r2,  c4.z, r4   ; Add resulting coordinates to r4
sub r3, v10, r1         ; Get difference in normal
mad r5, r3,  c4.z, r5   ; Add resulting normal to r5

; Get differences from 4th blended mesh and add into result
sub r2, v12, r0         ; Get difference in coordinates
mad r4, r2,  c4.w, r4   ; Add resulting coordinates to r4
sub r3, v13, r1         ; Get difference in normal
mad r5, r3,  c4.w, r5   ; Add resulting normal to r5

; Project position using world*view*projection transformation
m4x4 oPos, r4, c0

; Dot-normal normal with inversed light direction
dp3 oD0, r5, -c5

; Store texture coordinates
mov oT0.xy, v2
