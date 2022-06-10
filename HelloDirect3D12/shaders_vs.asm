//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// POSITION                 0   xyzw        0     NONE   float   xyzw
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float   xyzw
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
vs_5_0
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output_siv o0.xyzw, position
dcl_output o1.xyzw
dcl_temps 2
//
// Initial variable locations:
//   v0.x <- position.x; v0.y <- position.y; v0.z <- position.z; v0.w <- position.w; 
//   v1.x <- color.x; v1.y <- color.y; v1.z <- color.z; v1.w <- color.w; 
//   o1.x <- <VSMain return value>.color.x; o1.y <- <VSMain return value>.color.y; o1.z <- <VSMain return value>.color.z; o1.w <- <VSMain return value>.color.w; 
//   o0.x <- <VSMain return value>.position.x; o0.y <- <VSMain return value>.position.y; o0.z <- <VSMain return value>.position.z; o0.w <- <VSMain return value>.position.w
//
#line 22 "F:\Project\DirectX\Direct3D12Demo\HelloDirect3D12\shaders.hlsl"
mov r0.xyzw, v0.xyzw  // r0.x <- result.position.x; r0.y <- result.position.y; r0.z <- result.position.z; r0.w <- result.position.w

#line 23
mov r1.xyzw, v1.xyzw  // r1.x <- result.color.x; r1.y <- result.color.y; r1.z <- result.color.z; r1.w <- result.color.w

#line 25
mov o0.xyzw, r0.xyzw
mov o1.xyzw, r1.xyzw
ret 
// Approximately 5 instruction slots used