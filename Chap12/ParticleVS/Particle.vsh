; Billboard particle vertex shader (Particle.vsh)
; by Jim Adams, 2003
;
; v0  = Particle coordinates
; v1  = X/Y offsets to position vertex
; v2  = Diffuse color of particle
; v3  = Texture coordinates
;
; c0-c3   = view*projection matrix
; c4      = right direction
; c5      = up direction
vs.1.1

dcl_position  v0
dcl_position1 v1
dcl_color     v2
dcl_texcoord  v3

; Scale the corner's offsets by the right and up vectors
mov r2, v1
mad r1, r2.xxx, c4, v0
mad r1, r2.yyy, c5, r1

; Apply view * proj transformation
m4x4 oPos, r1, c0

; Store diffuse color
mov oD0, v2

; Store texture coordinates
mov oT0.xy, v3
