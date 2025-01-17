#include "renderer.h"

#include "shader.h"
#include "../../profile/game.h"
#include "../../game.h"
#include "3d/scene.h"
#include "yuvframe.h"
#include "../../../vendor/nuklear/nuklear_sdl_gl3.h"

namespace Impacto {
namespace OpenGL {

void Renderer::InitImpl() {
  if (IsInit) return;
  ImpLog(LL_Info, LC_Render, "Initializing Renderer2D system\n");
  IsInit = true;
  NuklearSupported = true;

  OpenGLWindow = new GLWindow();
  OpenGLWindow->Init();
  Window = (BaseWindow*)OpenGLWindow;

  Shaders = new ShaderCompiler();

  if (Profile::GameFeatures & GameFeature::Scene3D) {
    Scene = new Scene3D(OpenGLWindow, Shaders);
    Scene->Init();
  }

  // Fill index buffer with quads
  int index = 0;
  int vertex = 0;
  while (index + 6 <= IndexBufferCount) {
    // bottom-left -> top-left -> top-right
    IndexBuffer[index] = vertex + 0;
    IndexBuffer[index + 1] = vertex + 1;
    IndexBuffer[index + 2] = vertex + 2;
    // bottom-left -> top-right -> bottom-right
    IndexBuffer[index + 3] = vertex + 0;
    IndexBuffer[index + 4] = vertex + 2;
    IndexBuffer[index + 5] = vertex + 3;
    index += 6;
    vertex += 4;
  }

  // Generate buffers
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &IBO);
  glGenVertexArrays(1, &VAOSprites);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, VertexBufferSize, NULL, GL_STREAM_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               IndexBufferCount * sizeof(IndexBuffer[0]), IndexBuffer,
               GL_STATIC_DRAW);

  // Specify vertex layouts
  glBindVertexArray(VAOSprites);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexBufferSprites),
                        (void*)offsetof(VertexBufferSprites, Position));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexBufferSprites),
                        (void*)offsetof(VertexBufferSprites, UV));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBufferSprites),
                        (void*)offsetof(VertexBufferSprites, Tint));
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexBufferSprites),
                        (void*)offsetof(VertexBufferSprites, MaskUV));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  // Make 1x1 white pixel for colored rectangles
  Texture rectTexture;
  rectTexture.Load1x1(0xFF, 0xFF, 0xFF, 0xFF);
  SpriteSheet rectSheet(1.0f, 1.0f);
  rectSheet.Texture = rectTexture.Submit();
  RectSprite = Sprite(rectSheet, 0.0f, 0.0f, 1.0f, 1.0f);

  // Set up sprite shader
  ShaderProgramSprite = Shaders->Compile("Sprite");
  glUniform1i(glGetUniformLocation(ShaderProgramSprite, "ColorMap"), 0);
  ShaderProgramSpriteInverted = Shaders->Compile("Sprite_inverted");
  glUniform1i(glGetUniformLocation(ShaderProgramSpriteInverted, "ColorMap"), 0);
  ShaderProgramMaskedSprite = Shaders->Compile("MaskedSprite");
  glUniform1i(glGetUniformLocation(ShaderProgramMaskedSprite, "ColorMap"), 0);
  MaskedIsInvertedLocation =
      glGetUniformLocation(ShaderProgramMaskedSprite, "IsInverted");
  MaskedIsSameTextureLocation =
      glGetUniformLocation(ShaderProgramMaskedSprite, "IsSameTexture");
  ShaderProgramYUVFrame = Shaders->Compile("YUVFrame");
  glUniform1i(glGetUniformLocation(ShaderProgramYUVFrame, "Luma"), 0);
  YUVFrameCbLocation = glGetUniformLocation(ShaderProgramYUVFrame, "Cb");
  YUVFrameCrLocation = glGetUniformLocation(ShaderProgramYUVFrame, "Cr");
  YUVFrameIsAlphaLocation =
      glGetUniformLocation(ShaderProgramYUVFrame, "IsAlpha");
  ShaderProgramCCMessageBox = Shaders->Compile("CCMessageBoxSprite");
  glUniform1i(glGetUniformLocation(ShaderProgramCCMessageBox, "ColorMap"), 0);
  ShaderProgramCHLCCMenuBackground = Shaders->Compile("CHLCCMenuBackground");

  // No-mipmapping sampler
  glGenSamplers(1, &Sampler);
  glSamplerParameteri(Sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glSamplerParameteri(Sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(Sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
}

void Renderer::ShutdownImpl() {
  if (!IsInit) return;
  if (VBO) glDeleteBuffers(1, &VBO);
  if (IBO) glDeleteBuffers(1, &IBO);
  if (VAOSprites) glDeleteVertexArrays(1, &VAOSprites);
  if (RectSprite.Sheet.Texture) glDeleteTextures(1, &RectSprite.Sheet.Texture);
  IsInit = false;

  if (Profile::GameFeatures & GameFeature::Scene3D) {
    Scene->Shutdown();
  }
}

void Renderer::NuklearInitImpl() {
  Nk = nk_sdl_init(Window->SDLWindow, NkMaxVertexMemory, NkMaxElementMemory);
  struct nk_font_atlas* atlas;
  nk_sdl_font_stash_begin(&atlas);
  // no fonts => default font used, but we still have do the setup
  nk_sdl_font_stash_end();
}

void Renderer::NuklearShutdownImpl() { nk_sdl_shutdown(); }

int Renderer::NuklearHandleEventImpl(SDL_Event* ev) {
  SDL_Event e_nk;
  memcpy(&e_nk, ev, sizeof(SDL_Event));
  Window->AdjustEventCoordinatesForNk(&e_nk);
  return nk_sdl_handle_event(&e_nk);
}

void Renderer::BeginFrameImpl() {}

void Renderer::BeginFrame2DImpl() {
  if (Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->BeginFrame() called before EndFrame()\n");
    return;
  }

  Drawing = true;
  CurrentTexture = 0;
  CurrentMode = R2D_None;
  VertexBufferFill = 0;
  IndexBufferFill = 0;

  glDisable(GL_CULL_FACE);

  // TODO should we really be making this global?
  glBindSampler(0, Sampler);
}

void Renderer::EndFrameImpl() {
  if (!Drawing) return;
  Flush();
  Drawing = false;

  glBindSampler(0, 0);
}

uint32_t Renderer::SubmitTextureImpl(TexFmt format, uint8_t* buffer, int width,
                                     int height) {
  uint32_t result;
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);

  // Anisotropic filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

  // Load in data
  GLuint texFormat;
  switch (format) {
    case TexFmt_RGBA:
      texFormat = GL_RGBA;
      break;
    case TexFmt_RGB:
      texFormat = GL_RGB;
      break;
    case TexFmt_U8:
      texFormat = GL_RED;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, texFormat, width, height, 0, texFormat,
               GL_UNSIGNED_BYTE, buffer);

  // Build mip chain
  // TODO do this ourselves outside of Submit(), this can easily cause a
  // framedrop
  glGenerateMipmap(GL_TEXTURE_2D);

  return result;
}

void Renderer::FreeTextureImpl(uint32_t id) { glDeleteTextures(1, &id); }

YUVFrame* Renderer::CreateYUVFrameImpl(int width, int height) {
  auto frame = new GLYUVFrame();
  frame->Init(width, height);
  return (YUVFrame*)frame;
}

void Renderer::DrawRectImpl(RectF const& dest, glm::vec4 color, float angle) {
  DrawSprite(RectSprite, dest, color, angle);
}

void Renderer::DrawSprite3DRotatedImpl(Sprite const& sprite, RectF const& dest,
                                       float depth, glm::vec2 vanishingPoint,
                                       bool stayInScreen, glm::quat rot,
                                       glm::vec4 tint, bool inverted) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawSprite3DRotated() called before BeginFrame()\n");
    return;
  }

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  // Are we in sprite mode?
  EnsureModeSprite(inverted);

  // Do we have the texture assigned?
  EnsureTextureBound(sprite.Sheet.Texture);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  QuadSetUV(sprite.Bounds, sprite.Sheet.DesignWidth, sprite.Sheet.DesignHeight,
            (uintptr_t)&vertices[0].UV, sizeof(VertexBufferSprites));
  QuadSetPosition3DRotated(dest, depth, vanishingPoint, stayInScreen, rot,
                           (uintptr_t)&vertices[0].Position,
                           sizeof(VertexBufferSprites));

  for (int i = 0; i < 4; i++) vertices[i].Tint = tint;
}

void Renderer::DrawRect3DRotatedImpl(RectF const& dest, float depth,
                                     glm::vec2 vanishingPoint,
                                     bool stayInScreen, glm::quat rot,
                                     glm::vec4 color) {
  DrawSprite3DRotated(RectSprite, dest, depth, vanishingPoint, stayInScreen,
                      rot, color);
}

void Renderer::DrawCharacterMvlImpl(Sprite const& sprite, glm::vec2 topLeft,
                                    int verticesCount, float* mvlVertices,
                                    int indicesCount, uint16_t* mvlIndices,
                                    bool inverted, glm::vec4 tint) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawCharacterMvl() called before BeginFrame()\n");
    return;
  }

  // Draw just the character with this since we need to rebind the index buffer
  // anyway...
  Flush();

  // Do we have space for the whole character?
  EnsureSpaceAvailable(verticesCount, sizeof(VertexBufferSprites),
                       indicesCount);

  // Are we in sprite mode?
  EnsureModeSprite(inverted);

  // Do we have the texture assigned?
  EnsureTextureBound(sprite.Sheet.Texture);

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += verticesCount * sizeof(VertexBufferSprites);

  IndexBufferFill += indicesCount;

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(mvlIndices[0]),
               mvlIndices, GL_STATIC_DRAW);

  for (int i = 0; i < verticesCount; i++) {
    vertices[i].Position = DesignToNDC(glm::vec2(
        mvlVertices[i * 5] + topLeft.x, mvlVertices[i * 5 + 1] + topLeft.y));
    vertices[i].UV = glm::vec2(mvlVertices[i * 5 + 3], mvlVertices[i * 5 + 4]);
    vertices[i].Tint = tint;
  }

  // Flush again and bind back our buffer
  Flush();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               IndexBufferCount * sizeof(IndexBuffer[0]), IndexBuffer,
               GL_STATIC_DRAW);
}

void Renderer::DrawSpriteImpl(Sprite const& sprite, RectF const& dest,
                              glm::vec4 tint, float angle, bool inverted,
                              bool isScreencap) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawSprite() called before BeginFrame()\n");
    return;
  }

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  // Are we in sprite mode?
  EnsureModeSprite(inverted);

  if (isScreencap) {
    Flush();
  }

  // Do we have the texture assigned?
  EnsureTextureBound(sprite.Sheet.Texture);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  if (isScreencap) {
    QuadSetUVFlipped(sprite.Bounds, sprite.Sheet.DesignWidth,
                     sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
                     sizeof(VertexBufferSprites));
  } else {
    QuadSetUV(sprite.Bounds, sprite.Sheet.DesignWidth,
              sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
              sizeof(VertexBufferSprites));
  }
  QuadSetPosition(dest, angle, (uintptr_t)&vertices[0].Position,
                  sizeof(VertexBufferSprites));

  for (int i = 0; i < 4; i++) vertices[i].Tint = tint;
}

void Renderer::DrawMaskedSpriteImpl(Sprite const& sprite, Sprite const& mask,
                                    RectF const& dest, glm::vec4 tint,
                                    int alpha, int fadeRange, bool isScreencap,
                                    bool isInverted, bool isSameTexture) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawMaskedSprite() called before BeginFrame()\n");
    return;
  }

  if (alpha < 0) alpha = 0;
  if (alpha > fadeRange + 256) alpha = fadeRange + 256;

  float alphaRange = 256.0f / fadeRange;
  float constAlpha = ((255.0f - alpha) * alphaRange) / 255.0f;

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  Flush();
  CurrentMode = R2D_Masked;
  glBindVertexArray(VAOSprites);
  glUseProgram(ShaderProgramMaskedSprite);
  glUniform1i(glGetUniformLocation(ShaderProgramMaskedSprite, "Mask"), 2);
  glUniform2f(glGetUniformLocation(ShaderProgramMaskedSprite, "Alpha"),
              alphaRange, constAlpha);
  glUniform1i(MaskedIsInvertedLocation, isInverted);
  glUniform1i(MaskedIsSameTextureLocation, isSameTexture);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sprite.Sheet.Texture);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, mask.Sheet.Texture);
  glBindSampler(2, Sampler);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  if (isScreencap) {
    QuadSetUVFlipped(sprite.Bounds, sprite.Sheet.DesignWidth,
                     sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
                     sizeof(VertexBufferSprites));
  } else {
    QuadSetUV(sprite.Bounds, sprite.Sheet.DesignWidth,
              sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
              sizeof(VertexBufferSprites));
  }
  QuadSetUV(sprite.Bounds, sprite.Bounds.Width, sprite.Bounds.Height,
            (uintptr_t)&vertices[0].MaskUV, sizeof(VertexBufferSprites));

  QuadSetPosition(dest, 0.0f, (uintptr_t)&vertices[0].Position,
                  sizeof(VertexBufferSprites));

  for (int i = 0; i < 4; i++) vertices[i].Tint = tint;
}

void Renderer::DrawCCMessageBoxImpl(Sprite const& sprite, Sprite const& mask,
                                    RectF const& dest, glm::vec4 tint,
                                    int alpha, int fadeRange, float effectCt,
                                    bool isScreencap) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawCCMessageBox() called before BeginFrame()\n");
    return;
  }

  if (alpha < 0) alpha = 0;
  if (alpha > fadeRange + 256) alpha = fadeRange + 256;

  float alphaRange = 256.0f / fadeRange;
  float constAlpha = ((255.0f - alpha) * alphaRange) / 255.0f;

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  if (CurrentMode != R2D_CCMessageBox) {
    Flush();
    CurrentMode = R2D_CCMessageBox;
  }
  glBindVertexArray(VAOSprites);
  glUseProgram(ShaderProgramCCMessageBox);
  glUniform1i(glGetUniformLocation(ShaderProgramCCMessageBox, "Mask"), 2);
  glUniform4f(glGetUniformLocation(ShaderProgramCCMessageBox, "Alpha"),
              alphaRange, constAlpha, effectCt, 0.0f);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sprite.Sheet.Texture);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, mask.Sheet.Texture);
  glBindSampler(2, Sampler);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  if (isScreencap) {
    QuadSetUVFlipped(sprite.Bounds, sprite.Sheet.DesignWidth,
                     sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
                     sizeof(VertexBufferSprites));
  } else {
    QuadSetUV(sprite.Bounds, sprite.Sheet.DesignWidth,
              sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
              sizeof(VertexBufferSprites));
  }
  QuadSetUV(mask.Bounds, mask.Sheet.DesignWidth, mask.Sheet.DesignHeight,
            (uintptr_t)&vertices[0].MaskUV, sizeof(VertexBufferSprites));

  QuadSetPosition(dest, 0.0f, (uintptr_t)&vertices[0].Position,
                  sizeof(VertexBufferSprites));

  for (int i = 0; i < 4; i++) vertices[i].Tint = tint;
}

void Renderer::DrawCHLCCMenuBackgroundImpl(const Sprite& sprite,
                                           const Sprite& mask,
                                           const RectF& dest, float alpha) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawCHLCCMenuBackground() called before BeginFrame()\n");
    return;
  }

  if (alpha < 0.0f)
    alpha = 0;
  else if (alpha > 1.0f)
    alpha = 1.0f;

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  if (CurrentMode != R2D_CHLCCMenuBackground) {
    Flush();
    CurrentMode = R2D_CHLCCMenuBackground;
  }

  glBindVertexArray(VAOSprites);
  glUseProgram(ShaderProgramCHLCCMenuBackground);
  glUniform1i(glGetUniformLocation(ShaderProgramCCMessageBox, "ColorMap"), 0);
  glUniform1i(glGetUniformLocation(ShaderProgramCCMessageBox, "Mask"), 2);
  glUniform1f(glGetUniformLocation(ShaderProgramCHLCCMenuBackground, "Alpha"),
              alpha);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sprite.Sheet.Texture);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, mask.Sheet.Texture);
  glBindSampler(2, Sampler);

  // Do we have the texture assigned?
  EnsureTextureBound(sprite.Sheet.Texture);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  QuadSetUVFlipped(sprite.Bounds, sprite.Sheet.DesignWidth,
                   sprite.Sheet.DesignHeight, (uintptr_t)&vertices[0].UV,
                   sizeof(VertexBufferSprites));

  QuadSetUV(mask.Bounds, mask.Sheet.DesignWidth, mask.Sheet.DesignHeight,
            (uintptr_t)&vertices[0].MaskUV, sizeof(VertexBufferSprites));

  QuadSetPosition(dest, 0.0f, (uintptr_t)&vertices[0].Position,
                  sizeof(VertexBufferSprites));
}

inline void Renderer::QuadSetUVFlipped(RectF const& spriteBounds,
                                       float designWidth, float designHeight,
                                       uintptr_t uvs, int stride) {
  float topUV = (spriteBounds.Y / designHeight);
  float leftUV = (spriteBounds.X / designWidth);
  float bottomUV = ((spriteBounds.Y + spriteBounds.Height) / designHeight);
  float rightUV = ((spriteBounds.X + spriteBounds.Width) / designWidth);

  // top-left
  *(glm::vec2*)(uvs + 0 * stride) = glm::vec2(leftUV, topUV);
  // bottom-left
  *(glm::vec2*)(uvs + 1 * stride) = glm::vec2(leftUV, bottomUV);
  // bottom-right
  *(glm::vec2*)(uvs + 2 * stride) = glm::vec2(rightUV, bottomUV);
  // top-right
  *(glm::vec2*)(uvs + 3 * stride) = glm::vec2(rightUV, topUV);
}

inline void Renderer::QuadSetUV(RectF const& spriteBounds, float designWidth,
                                float designHeight, uintptr_t uvs, int stride) {
  float topUV = (spriteBounds.Y / designHeight);
  float leftUV = (spriteBounds.X / designWidth);
  float bottomUV = ((spriteBounds.Y + spriteBounds.Height) / designHeight);
  float rightUV = ((spriteBounds.X + spriteBounds.Width) / designWidth);

  // bottom-left
  *(glm::vec2*)(uvs + 0 * stride) = glm::vec2(leftUV, bottomUV);
  // top-left
  *(glm::vec2*)(uvs + 1 * stride) = glm::vec2(leftUV, topUV);
  // top-right
  *(glm::vec2*)(uvs + 2 * stride) = glm::vec2(rightUV, topUV);
  // bottom-right
  *(glm::vec2*)(uvs + 3 * stride) = glm::vec2(rightUV, bottomUV);
}

inline void Renderer::QuadSetPosition(RectF const& transformedQuad, float angle,
                                      uintptr_t positions, int stride) {
  glm::vec2 bottomLeft =
      glm::vec2(transformedQuad.X, transformedQuad.Y + transformedQuad.Height);
  glm::vec2 topLeft = glm::vec2(transformedQuad.X, transformedQuad.Y);
  glm::vec2 topRight =
      glm::vec2(transformedQuad.X + transformedQuad.Width, transformedQuad.Y);
  glm::vec2 bottomRight = glm::vec2(transformedQuad.X + transformedQuad.Width,
                                    transformedQuad.Y + transformedQuad.Height);

  if (angle != 0.0f) {
    glm::vec2 center = transformedQuad.Center();
    glm::mat2 rot = Rotate2D(angle);

    bottomLeft = rot * (bottomLeft - center) + center;
    topLeft = rot * (topLeft - center) + center;
    topRight = rot * (topRight - center) + center;
    bottomRight = rot * (bottomRight - center) + center;
  }

  // bottom-left
  *(glm::vec2*)(positions + 0 * stride) = DesignToNDC(bottomLeft);
  // top-left
  *(glm::vec2*)(positions + 1 * stride) = DesignToNDC(topLeft);
  // top-right
  *(glm::vec2*)(positions + 2 * stride) = DesignToNDC(topRight);
  // bottom-right
  *(glm::vec2*)(positions + 3 * stride) = DesignToNDC(bottomRight);
}

void Renderer::QuadSetPosition3DRotated(RectF const& transformedQuad,
                                        float depth, glm::vec2 vanishingPoint,
                                        bool stayInScreen, glm::quat rot,
                                        uintptr_t positions, int stride) {
  float widthNormalized = transformedQuad.Width / (Profile::DesignWidth * 0.5f);
  float heightNormalized =
      transformedQuad.Height / (Profile::DesignHeight * 0.5f);

  glm::vec4 corners[4]{
      // bottom-left
      {glm::vec2(-widthNormalized / 2.0f, -heightNormalized / 2.0f), 0, 1},
      // top-left
      {glm::vec2(-widthNormalized / 2.0f, heightNormalized / 2.0f), 0, 1},
      // top-right
      {glm::vec2(widthNormalized / 2.0f, heightNormalized / 2.0f), 0, 1},
      // bottom-right
      {glm::vec2(widthNormalized / 2.0f, -heightNormalized / 2.0f), 0, 1}};

  glm::mat4 transform =
      glm::translate(glm::mat4(1.0f),
                     glm::vec3(DesignToNDC(transformedQuad.Center()), 0)) *
      glm::mat4_cast(rot);

  glm::vec4 vanishingPointNDC(DesignToNDC(vanishingPoint), 0, 0);

  for (int i = 0; i < 4; i++) {
    corners[i] = transform * corners[i];
  }

  if (stayInScreen) {
    float maxZ = 0.0f;
    for (int i = 0; i < 4; i++) {
      if (corners[i].z > maxZ) maxZ = corners[i].z;
    }
    for (int i = 0; i < 4; i++) {
      corners[i].z -= maxZ;
    }
  }

  for (int i = 0; i < 4; i++) {
    // perspective
    corners[i] -= vanishingPointNDC;
    corners[i].x *= (depth / (depth - corners[i].z));
    corners[i].y *= (depth / (depth - corners[i].z));
    corners[i] += vanishingPointNDC;

    *(glm::vec2*)(positions + i * stride) = corners[i];
  }
}

void Renderer::EnsureSpaceAvailable(int vertices, int vertexSize, int indices) {
  if (VertexBufferFill + vertices * vertexSize > VertexBufferSize ||
      IndexBufferFill + indices > IndexBufferCount) {
    ImpLogSlow(
        LL_Trace, LC_Render,
        "Renderer->EnsureSpaceAvailable flushing because buffers full at "
        "VertexBufferFill=%08X,IndexBufferFill=%08X\n",
        VertexBufferFill, IndexBufferFill);
    Flush();
  }
}

void Renderer::EnsureTextureBound(GLuint texture) {
  if (CurrentTexture != texture) {
    ImpLogSlow(LL_Trace, LC_Render,
               "Renderer->EnsureTextureBound flushing because texture %d is "
               "not %d\n",
               CurrentTexture, texture);
    Flush();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    CurrentTexture = texture;
  }
}

void Renderer::EnsureModeSprite(bool inverted) {
  Renderer2DMode wantedMode = inverted ? R2D_SpriteInverted : R2D_Sprite;
  if (CurrentMode != wantedMode) {
    ImpLogSlow(
        LL_Trace, LC_Render,
        "Renderer2D flushing because mode %d is not R2D_Sprite/inverted\n",
        CurrentMode);
    Flush();
    glBindVertexArray(VAOSprites);
    glUseProgram(inverted ? ShaderProgramSpriteInverted : ShaderProgramSprite);
    CurrentMode = wantedMode;
  }
}

void Renderer::Flush() {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->Flush() called before BeginFrame()\n");
    return;
  }
  if (VertexBufferFill > 0 && IndexBufferFill > 0) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // TODO: better to specify the whole thing or just this?
    glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferFill, VertexBuffer);
    glDrawElements(GL_TRIANGLES, IndexBufferFill, GL_UNSIGNED_SHORT, 0);
  }
  IndexBufferFill = 0;
  VertexBufferFill = 0;
  CurrentTexture = 0;
}

void Renderer::DrawVideoTextureImpl(YUVFrame* tex, RectF const& dest,
                                    glm::vec4 tint, float angle,
                                    bool alphaVideo) {
  if (!Drawing) {
    ImpLog(LL_Error, LC_Render,
           "Renderer->DrawVideoTexture() called before BeginFrame()\n");
    return;
  }

  // Do we have space for one more sprite quad?
  EnsureSpaceAvailable(4, sizeof(VertexBufferSprites), 6);

  if (CurrentMode != R2D_YUVFrame) {
    Flush();
    CurrentMode = R2D_YUVFrame;
  }
  glBindVertexArray(VAOSprites);
  glUseProgram(ShaderProgramYUVFrame);
  glUniform1i(YUVFrameCbLocation, 2);
  glUniform1i(YUVFrameCrLocation, 4);
  glUniform1i(YUVFrameIsAlphaLocation, alphaVideo);

  // Luma
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex->LumaId);

  // Cb
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, tex->CbId);
  glBindSampler(2, Sampler);

  // Cr
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, tex->CrId);
  glBindSampler(4, Sampler);

  // OK, all good, make quad

  VertexBufferSprites* vertices =
      (VertexBufferSprites*)(VertexBuffer + VertexBufferFill);
  VertexBufferFill += 4 * sizeof(VertexBufferSprites);

  IndexBufferFill += 6;

  QuadSetUV(RectF(0.0f, 0.0f, tex->Width, tex->Height), tex->Width, tex->Height,
            (uintptr_t)&vertices[0].UV, sizeof(VertexBufferSprites));
  QuadSetPosition(dest, angle, (uintptr_t)&vertices[0].Position,
                  sizeof(VertexBufferSprites));

  for (int i = 0; i < 4; i++) vertices[i].Tint = tint;
}

void Renderer::CaptureScreencapImpl(Sprite const& sprite) {
  Flush();
  Window->SwapRTs();
  int prevTextureBinding;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTextureBinding);
  glBindTexture(GL_TEXTURE_2D, sprite.Sheet.Texture);
  glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, Window->WindowWidth,
                   Window->WindowHeight, 0);
  glBindTexture(GL_TEXTURE_2D, prevTextureBinding);
  Window->SwapRTs();
}

void Renderer::EnableScissorImpl() {
  Flush();
  glEnable(GL_SCISSOR_TEST);
}

void Renderer::SetScissorRectImpl(RectF const& rect) {
  Rect viewport = Window->GetViewport();
  glScissor((GLint)(rect.X),
            (GLint)((viewport.Height - (GLint)(rect.Y + rect.Height))),
            (GLint)(rect.Width), (GLint)(rect.Height));
}

void Renderer::DisableScissorImpl() {
  Flush();
  glDisable(GL_SCISSOR_TEST);
}

}  // namespace OpenGL
}  // namespace Impacto
