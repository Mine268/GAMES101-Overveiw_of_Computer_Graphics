# GAMES101 Assignment3 作业框架分析

写作业之前看了一眼代码框架，发现疑问挺多的，多方查证之后基本得到了解决。本文主要分析一下 `rst::rasterizer::draw` 和 `rst::rasterizer::rasterize_triangle` 方法的内容。

## draw 方法

首先这个方法会接受一个三角形片元的数组 `TriangleList`，这个数组包含了需要光栅化的模型的每一个面，这个方法将会对这个数组中的每个三角形进行光栅化。每个三角形都存储了自己的位置、颜色、uv、法线等等信息，这些就是进行光栅化的参数依据。

在方法内部，for循环依次提取每一个三角形进行光栅化。依据代码中的命名，我们把每一次进行光栅化的三角形命名为 `t`。

```cpp
for (const auto& t : TriangleList) { /* ... */ }
```

