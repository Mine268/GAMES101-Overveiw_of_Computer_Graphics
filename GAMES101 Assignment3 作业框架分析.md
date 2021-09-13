# GAMES101 Assignment3 作业框架分析

写作业之前看了一眼代码框架，发现疑问挺多的，多方查证之后基本得到了解决。本文主要分析一下 `rst::rasterizer::draw` 和 `rst::rasterizer::rasterize_triangle` 方法的内容。

这一篇分析参考了网上的资料，代码部分参考了github以为大佬的代码。但是我没有保存具体的地址QAQ

## `rasterizer::draw`

首先这个方法会接受一个三角形片元的数组 `TriangleList`，这个数组包含了需要光栅化的模型的每一个面，这个方法将会对这个数组中的每个三角形进行光栅化。每个三角形都存储了自己的位置、颜色、uv、法线等等信息，这些就是进行光栅化的参数依据。

在方法内部，for循环依次提取每一个三角形进行光栅化。依据代码中的命名，我们把每一次进行光栅化的三角形命名为 `t`。

```cpp
for (const auto& t : TriangleList) { /* ... */ }
```

每个三角形在进行正式的光栅化之前都至少需要经过 `model, view, projection` 变换，由于投影变换会改变某一些透视关系不便于进行光线折射的计算，所以我们将经过了变换 `model, view` 变换的三角形坐标存储在数组 `viewspace_pos` 中

```cpp
std::array<Eigen::Vector4f, 3> mm {
    (view * model * t->v[0]),
    (view * model * t->v[1]),
    (view * model * t->v[2])
}; // 我也不知道mm有什么用，但既然这里写了我还是留着

std::array<Eigen::Vector3f, 3> viewspace_pos;

std::transform(mm.begin(), mm.end(), viewspace_pos.begin(), [](auto& v) {
    return v.template head<3>();
});
```

同时，将经过了 `model, view, projection` 变换的三角形坐标存入到数组 `v` 中，并进行齐次除法

```cpp
Eigen::Vector4f v[] = {
    mvp * t->v[0],
    mvp * t->v[1],
    mvp * t->v[2]
};
//Homogeneous division
for (auto& vec : v) {
    vec.x()/=vec.w();
    vec.y()/=vec.w();
    vec.z()/=vec.w();
}
```

这里展开说明一下，使用的投影矩阵为
$$
\begin{bmatrix}
n&0&0&0\\
0&n&0&0\\
0&0&n+f&nf\\
0&0&-1&0
\end{bmatrix}
$$
那么对于每一个 `v` 中的元素 `v[i]`，其 `x,y,z` 坐标都对应了进行MVP变换之后的坐标，但是 `w` 坐标保存了进行P变换的 `z` 坐标，这一点对于我们后续进行插值运算十分重要，这也是为何没有在齐次除法的时候进行

```cpp
vec.w() /= vec.w(); // 齐次除法只是对于xyz而言的，这样做也没有意义
```

的原因。

然后进入到这一步

```cpp
Eigen::Matrix4f inv_trans = (view * model).inverse().transpose();
Eigen::Vector4f n[] = {
    inv_trans * to_vec4(t->normal[0], 0.0f),
    inv_trans * to_vec4(t->normal[1], 0.0f),
    inv_trans * to_vec4(t->normal[2], 0.0f)
};
```

这一步是计算进行了 `model, view` 变换之后的每个点上的法线向量，这一步对于后续正确计算三种光线折射十分重要，因为法线在变换的过程中可能无法点的切线向量（如果存在切线数据的话）保持垂直。我们假设某一点的法线向量为 $n$，切线向量为 $t$，简化 `model, view` 变换矩阵为 $M$。在原有的模型中
$$
n^T t=0
$$
经过变换之后，切线向量变成了
$$
Mt
$$
按照法线的定义，变换之后的法线向量 $n'$有
$$
n'^TMt=0
$$
由于 $n^Tt=n^TM^{-1}Mt=0$，所以
$$
n'^T=n^TM^{-1}=((M^{-1})^Tn)^T\Rightarrow n'=(M^{-1})^Tn
$$
所以经过了 `model, view` 变换的点的法线向量并不是 $\text{View}\cdot\text{Model}\cdot n$，而是
$$
((\text{View}\cdot\text{Model})^{-1})^T\cdot n
$$
这也是此处代码这么书写的原因。

然后是视口变换的代码，不做说明。

```cpp
//Viewport transformation
for (auto & vert : v)
{
    vert.x() = 0.5*width*(vert.x()+1.0);
    vert.y() = 0.5*height*(vert.y()+1.0);
    vert.z() = vert.z() * f1 + f2;
}
```

接着我们将进行了MVP变换、齐次处理和视口变换的三角新坐标填入三角形 `newtri`，这个坐标是在屏幕空间中的坐标，带有 `z` 坐标的原因是为了进行 `z-buffer`

```cpp
for (int i = 0; i < 3; ++i)
{
    //screen space coordinates
    newtri.setVertex(i, v[i]);
}
```

然后填入正确变换的法线向量坐标，这个坐标是在摄像机空间中的坐标，也就是进行了 `model, view` 变换但是没有进行 `projection` 变换的坐标，法线向量进行投影变换是没有意义的。

```cpp
for (int i = 0; i < 3; ++i)
{
    //view space normal
    newtri.setNormal(i, n[i].head<3>());
}
```

下一步设置三点的颜色，如果我们的片元着色器是使用 `uv` 获取颜色的那么这一步并不会起作用。

```cpp
newtri.setColor(0, 148,121.0,92.0);
newtri.setColor(1, 148,121.0,92.0);
newtri.setColor(2, 148,121.0,92.0);
```

最后光栅化这个处理好的三角形。

## `rasterizer::rasterize_triangle`

和作业2中类似，我们首先框定三角形的范围，在这个范围内进行扫描。

```cpp
std::array<Eigen::Vector4f, 3ULL> v = t.toVector4();
int minx = fmin(v[0].x(), fmin(v[1].x(), v[2].x()));
int maxx = fmax(v[0].x(), fmax(v[1].x(), v[2].x()));
int miny = fmin(v[0].y(), fmin(v[1].y(), v[2].y()));
int maxy = fmax(v[0].y(), fmax(v[1].y(), v[2].y()));
```

这里框定了三角形占据的最高最低、最左最右的 `y` 和 `x`

然后就是光栅化这个三角形的核心代码，这里全部贴上，然后分段分析

```cpp
for (int vx = minx; vx <= maxx; ++vx) {
    for (int vy = miny; vy <= maxy; ++vy) {
        if (insideTriangle(vx, vy, t.v)) {
            auto [alpha, beta, gamma] = computeBarycentric2D(vx, vy, t.v);
            float Z = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            float zp = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            zp *= Z;

            auto interpolated_color = (alpha * t.color[0] / v[0].w() +
                                       beta * t.color[1] / v[1].w() +
                                       gamma * t.color[2] / v[2].w()) *
                Z;
            auto interpolated_normal =
                (alpha * t.normal[0] / v[0].w() +
                 beta * t.normal[1] / v[1].w() +
                 gamma * t.normal[2] / v[2].w()) *
                Z;
            auto interpolated_texcoords =
                (alpha * t.tex_coords[0] / v[0].w() +
                 beta * t.tex_coords[1] / v[1].w() +
                 gamma * t.tex_coords[2] / v[2].w()) *
                Z;
            auto interpolated_shadingcoords =
                (alpha * view_pos[0] / v[0].w() +
                 beta * view_pos[1] / v[1].w() +
                 gamma * view_pos[2] / v[2].w()) *
                Z;

            fragment_shader_payload payload(interpolated_color, interpolated_normal.normalized(), interpolated_texcoords
                                            , texture ? &*texture : nullptr);
            payload.view_pos = interpolated_shadingcoords;
            auto pixel_color = fragment_shader(payload);
            set_pixel(Eigen::Vector2i(vx, vy), pixel_color);
            }
        }
    }
}
```

两层for循环不必多说，用来遍历框定的范围。接着通过三次叉乘的 `insideTriangle` 判断这一点是否属于这个三角形片元，这里是在进行了MVP变换之后的三角形坐标的基础上进行的判断，但是这样的判断是准确的，不必担心。如果该点在三角形片元内，也就是在摄像机空间中的三角形上的话，就需要进行插值运算，插值运算的目的是求出

- 摄像机空间中这一点的 `z` 值，用于进行 `z-buffer`
- 摄像机空间中这一点的属性值，包括
  - 法线向量
  - `uv`
  - 颜色
  - `x,y,z`

而插值的方式是通过三角形重心坐标，重心坐标的定义这里不做赘述，直接看到这一行代码

```cpp
auto [alpha, beta, gamma] = computeBarycentric2D(vx, vy, t.v);
```

这一行代码的含义是通过当前正扫描到的像素点的 $x,y$ 计算这一点在三角形片元上的重心坐标，这个三角形片元是原来的摄像机空间中的三角形进行了MVP变换之后形成的。可以在源代码中找到计算这个重心坐标的代码

```cpp
static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector4f* v){
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}
```

这里给出推导，我们设三角形片元三点坐标分别为 $(x_i,y_i.z_i,w_i),i=1,2,3$，前三个坐标是进行过齐次除法的，所以可以直接拿过来用。当前扫描到三角形内部的点的横纵坐标分别为 $x,y$，其对应的重心坐标为 $\alpha',\beta',\gamma'$，有
$$
\left\{
\begin{aligned}
x&=\alpha'x_1+\beta'x_2+\gamma'x_3\\
y&=\alpha'y_1+\beta'y_2+\gamma'y_3
\end{aligned}
\right.\Rightarrow
\left\{
\begin{aligned}
x-x_3&=(x_1-x_3)\,\alpha'+(x_1-x_2)\,\beta'\\
y-y_3&=(y_1-y_3)\,\alpha'+(y_1-y_2)\,\beta'
\end{aligned}
\right.
$$
解得
$$
\left\{
\begin{aligned}
\alpha'&=\frac{(x-x_3)(y_1-y_2)-(x_1-x_2)(y-y_3)}{(x_1-x_3)(y_1-y_2)-(x_1-x_2)(y_1-y_3)}\\
\beta'&=\frac{(x-x_3)-(x_1-x_3)\,\alpha'}{x_1-x_2}\\
\gamma'&=1-\alpha'-\beta'
\end{aligned}
\right.
$$
展开后即是代码中缩写，但是以上的这种方式更加简便

通过这种方式，我们计算得到了当前扫描到的三角形片元中的点在这个三角形片元中的重心坐标，但是需要注意的一点是，经过 `projection` 变换，三角形重心坐标并不会保持不变，换句话说就是变换前后的点在三角形中的重心坐标并不是不变的，我们计算得到的重心坐标是变换后的三角形片元中的坐标，并不能直接用于插值，需要在计算出摄像机空间中这一点的重心坐标 $\alpha,\beta,\gamma$ 后，依据这个重心坐标才能够进行插值计算，这也是下面这一段代码所作的事情

```cpp
float Z = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
float zp = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
zp *= Z;
```

我们假设三角形和其中的一点的 `z` 坐标分别为 $z_1,z_2,z_3,z$，其进行投影变换后的坐标为 $z_1',z_2',z_3',z'$，重心坐标分别为 $(\alpha,\beta,\gamma),(\alpha',\beta',\gamma')$，依据定义，我们有
$$
\left\{
\begin{aligned}
z&=\begin{bmatrix}z_1&z_2&z_3\end{bmatrix}\begin{bmatrix}\alpha\\\beta\\\gamma\end{bmatrix}\\
z'&=\begin{bmatrix}z_1'&z_2'&z_3'\end{bmatrix}\begin{bmatrix}\alpha'\\\beta'\\\gamma'\end{bmatrix}
\end{aligned}
\right.
$$
根据重心坐标的定义
$$
1=\alpha'+\beta'+\gamma'
$$
显然
$$
1=\frac zz=\frac{z_1}{z_1}\alpha+\frac{z_2}{z_2}\beta+\frac{z_3}{z_3}\gamma\Rightarrow
z=\begin{bmatrix}z_1&z_2&z_3\end{bmatrix}\begin{bmatrix}\alpha'z/z_1\\\beta'z/z_2\\\gamma'z/z_3\end{bmatrix}
$$
结合前文中 $z$ 的定义，有
$$
\left\{
\begin{aligned}
\alpha&=\frac{z}{z_1}\alpha'\\
\beta&=\frac{z}{z_2}\beta'\\
\gamma&=\frac{z}{z_3}\gamma'\\
\end{aligned}
\right.
$$
又
$$
1=\alpha+\beta+\gamma=\frac{z}{z_1}\alpha'+\frac{z}{z_2}\beta'+\frac{z}{z_3}\gamma'
$$
故
$$
z=\cfrac1{\cfrac{\alpha'}{z_1}+\cfrac{\beta'}{z_2}+\cfrac{\gamma'}{z_3}}
$$
对于我们需要插值的任意属性，其在三角形三点的值为 $I_1, I_2, I_3$，我们需要计算其中某一点的插值 $I$，若计算出在投影后的插值 $I'$ 和重心坐标 $(\alpha',\beta',\gamma')$，在已知三点的投影坐标 $(x'_i,y'_i,z'_i,w'_i),i=1,2,3$ 的情况下，我们可以反推出 $I$

我们设这一点在投影前的重心坐标 $(\alpha,\beta,\gamma)$，那么
$$
\begin{aligned}
I&=\begin{bmatrix}I_1&I_2&I_3\end{bmatrix}\begin{bmatrix}\alpha\\\beta\\\gamma\end{bmatrix}\\
&=\begin{bmatrix}I_1&I_2&I_3\end{bmatrix}\begin{bmatrix}\alpha'z/z_1\\\beta'z/z_2\\\gamma'z/z_3\end{bmatrix}\\
&=\begin{bmatrix}\cfrac{z}{z_1}I_1&\cfrac{z}{z_2}I_2&\cfrac{z}{z_3}I_3\end{bmatrix}\begin{bmatrix}\alpha'\\\beta'\\\gamma'\end{bmatrix}\\
&=\left(\alpha'\frac{I_1}{z_1}+\beta'\frac{I_2}{z_2}+\gamma'\frac{I_3}{z_3}\right)\,z
\end{aligned}
$$
由此以下的插值代码也就容易理解了

```cpp
auto interpolated_color = (alpha * t.color[0] / v[0].w() +
                           beta * t.color[1] / v[1].w() +
                           gamma * t.color[2] / v[2].w()) *
    					Z;
auto interpolated_normal =
    (alpha * t.normal[0] / v[0].w() +
     beta * t.normal[1] / v[1].w() +
     gamma * t.normal[2] / v[2].w()) *
    Z;
auto interpolated_texcoords =
    (alpha * t.tex_coords[0] / v[0].w() +
     beta * t.tex_coords[1] / v[1].w() +
     gamma * t.tex_coords[2] / v[2].w()) *
    Z;
auto interpolated_shadingcoords =
    (alpha * view_pos[0] / v[0].w() +
     beta * view_pos[1] / v[1].w() +
     gamma * view_pos[2] / v[2].w()) *
    Z;
```

最后，用插值得到的这一点的颜色、法线向量、`uv` 和模型的材质形成一个片元传入给片元着色器，片元着色器根据编写好的着色方式计算出这一点的最终颜色，然后再绘制在屏幕上对应像素点处。

```cpp
fragment_shader_payload payload(interpolated_color, interpolated_normal.normalized(), interpolated_texcoords , texture ? &*texture : nullptr);
payload.view_pos = interpolated_shadingcoords;
auto pixel_color = fragment_shader(payload);
set_pixel(Eigen::Vector2i(vx, vy), pixel_color);
```

