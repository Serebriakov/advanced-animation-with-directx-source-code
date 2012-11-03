; Billboard particle vertex shader (Particle.vsh)
; by Jim Adams, 2002
;
; v0  = Particle coordinates
; v1  = X/Y offsets to position vertex
; v2  = Diffuse color of particle
; v3  = Texture coordinates
;
; c0-c3   = world*view*projection matrix
; c4-c7   = inverted view matrix
vs.1.0

dcl_position  v0
dcl_position1 v1
dcl_color     v2
dcl_texcoord  v3

; Transform offset coordinates by inverted view transformation
m4x4 r1, v1, c4

; Add in world coordinates
add r1, r1, v0

; Project position using world*view*projection transformation
m4x4 oPos, r1, c0

; Store diffuse color
mov oD0, v2

; Store texture coordinates
mov oT0.xy, v3
