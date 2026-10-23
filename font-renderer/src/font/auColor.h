#pragma once

struct auColor {
  float r, g, b, a;

  auColor() : r(1.f), g(1.f), b(1.f), a(1.f) {};
  auColor(double r, double g, double b, double a = 1.f)
      : r(r), g(g), b(b), a(a) {};

  static const auColor Black;
  static const auColor White;
  static const auColor Red;
  static const auColor Green;
  static const auColor Blue;
  static const auColor Transparent;
};
