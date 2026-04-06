/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#include "M3Exporter.h"

#include <algorithm>
#include <format>
#include <string>

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../buffer.h"
#include "../../casc/export-helper.h"
#include "../loaders/M3Loader.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../writers/GLTFWriter.h"
#include "../writers/JSONWriter.h"

M3Exporter::M3Exporter(BufferWrapper data, std::vector<uint32_t> variantTextures, uint32_t fileDataID)
: data(std::move(data))
, fileDataID(fileDataID)
, variantTextures(std::move(variantTextures))
{
m3 = std::make_unique<M3Loader>(this->data);
}

/**
 * Set the mask array used for geoset control.
 * @param mask
 */
void M3Exporter::setGeosetMask(std::vector<M3ExportGeosetMask> mask) {
geosetMask = std::move(mask);
}

/**
 * Export additional texture from canvas
 */
void M3Exporter::addURITexture(const std::string& out, BufferWrapper pngData) {
dataTextures.emplace(out, std::move(pngData));
}

/**
 * Export the textures for this M2 model.
 * @param out
 * @param raw
 * @param mtl
 * @param helper
 * @param fullTexPaths
 * @returns Texture map
 */
std::map<uint32_t, std::string> M3Exporter::exportTextures(const std::filesystem::path& out, bool raw,
MTLWriter* mtl, casc::ExportHelper* helper, bool fullTexPaths)
{
const std::map<uint32_t, std::string> validTextures;
return validTextures;
}

void M3Exporter::exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format) {
const std::string ext = format == "glb" ? ".glb" : ".gltf";
const auto outGLTF = casc::ExportHelper::replaceExtension(out.string(), ext);

// Skip export if file exists and overwriting is disabled.
if (!core::view->config.value("overwriteFiles", true) && generics::fileExists(outGLTF)) {
std::string formatUpper = format;
std::transform(formatUpper.begin(), formatUpper.end(), formatUpper.begin(), ::toupper);
logging::write(std::format("Skipping {} export of {} (already exists, overwrite disabled)", formatUpper, outGLTF));
return;
}

m3->load();

const auto model_name = std::filesystem::path(outGLTF).stem().string();
GLTFWriter gltf(out, model_name);
{
std::string formatUpper = format;
std::transform(formatUpper.begin(), formatUpper.end(), formatUpper.begin(), ::toupper);
logging::write(std::format("Exporting M3 model {} as {}: {}", model_name, formatUpper, outGLTF));
}

gltf.setVerticesArray(m3->vertices);
gltf.setNormalArray(m3->normals);
gltf.addUVArray(m3->uv);

for (size_t mI = 0, mC = m3->geosets.size(); mI < mC; mI++) {
// Skip geosets that are not enabled.
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& geoset = m3->geosets[mI];

// Read geoset name from the string block
std::string geosetName;
if (m3->stringBlock && geoset.nameCharCount > 0) {
m3->stringBlock->seek(geoset.nameCharStart);
geosetName = m3->stringBlock->readString(geoset.nameCharCount);
}

if (geosetName.empty())
geosetName = std::to_string(mI);

std::vector<uint32_t> indices(geoset.indexCount);
for (uint32_t iI = 0; iI < geoset.indexCount; iI++)
indices[iI] = m3->indices[geoset.indexStart + iI];

gltf.addMesh(geosetName, indices, "");
}

gltf.write(core::view->config.value("overwriteFiles", true), format);
}

/**
 * Export the M3 model as a WaveFront OBJ.
 * @param out
 * @param exportCollision
 * @param helper
 * @param fileManifest
 */
void M3Exporter::exportAsOBJ(const std::filesystem::path& out, bool exportCollision,
casc::ExportHelper* helper, std::vector<M3ExportFileManifest>* fileManifest)
{
m3->load();

const auto& config = core::view->config;

OBJWriter obj(out);
auto mtlPath = casc::ExportHelper::replaceExtension(out.string(), ".mtl");
MTLWriter mtl(mtlPath);

const auto model_name = out.stem().string();
obj.setName(model_name);

logging::write(std::format("Exporting M3 model {} as OBJ: {}", model_name, out.string()));

obj.setVertArray(m3->vertices);
obj.setNormalArray(m3->normals);
obj.addUVArray(m3->uv);

// Textures
const auto outDir = out.parent_path();
exportTextures(outDir, false, &mtl, helper);

// Abort if the export has been cancelled.
if (helper && helper->isCancelled())
return;

// Faces
for (size_t mI = 0, mC = m3->geosets.size(); mI < mC; mI++) {
// Skip geosets that are not enabled.
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& geoset = m3->geosets[mI];

// Read geoset name from the string block
std::string geosetName;
if (m3->stringBlock && geoset.nameCharCount > 0) {
m3->stringBlock->seek(geoset.nameCharStart);
geosetName = m3->stringBlock->readString(geoset.nameCharCount);
}

if (geosetName.empty())
geosetName = std::to_string(mI);

std::vector<uint32_t> indices(geoset.indexCount);
for (uint32_t iI = 0; iI < geoset.indexCount; iI++)
indices[iI] = m3->indices[geoset.indexStart + iI];

obj.addMesh(geosetName, indices, "");
}

if (!mtl.isEmpty())
obj.setMaterialLibrary(std::filesystem::path(mtlPath).filename().string());

obj.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "OBJ", fileDataID, out });

mtl.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "MTL", fileDataID, std::filesystem::path(mtlPath) });
}

/**
 * Export the M3 model as an STL file.
 * @param out
 * @param exportCollision
 * @param helper
 * @param fileManifest
 */
void M3Exporter::exportAsSTL(const std::filesystem::path& out, bool exportCollision,
casc::ExportHelper* helper, std::vector<M3ExportFileManifest>* fileManifest)
{
m3->load();

const auto& config = core::view->config;

STLWriter stl(out);
const auto model_name = out.stem().string();
stl.setName(model_name);

logging::write(std::format("Exporting M3 model {} as STL: {}", model_name, out.string()));

stl.setVertArray(m3->vertices);
stl.setNormalArray(m3->normals);

// abort if the export has been cancelled
if (helper && helper->isCancelled())
return;

// faces
for (size_t mI = 0, mC = m3->geosets.size(); mI < mC; mI++) {
// skip geosets that are not enabled
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& geoset = m3->geosets[mI];

// Read geoset name from the string block
std::string geosetName;
if (m3->stringBlock && geoset.nameCharCount > 0) {
m3->stringBlock->seek(geoset.nameCharStart);
geosetName = m3->stringBlock->readString(geoset.nameCharCount);
}

if (geosetName.empty())
geosetName = std::to_string(mI);

std::vector<uint32_t> indices(geoset.indexCount);
for (uint32_t iI = 0; iI < geoset.indexCount; iI++)
indices[iI] = m3->indices[geoset.indexStart + iI];

stl.addMesh(geosetName, indices);
}

stl.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "STL", fileDataID, out });
}

/**
 * Export the model as a raw M3 file, including related files
 * such as textures, bones, animations, etc.
 * @param out
 * @param helper
 * @param fileManifest
 */
void M3Exporter::exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
std::vector<M3ExportFileManifest>* fileManifest)
{
// Write the M3 file with no conversion.
data.writeToFile(out);
if (fileManifest)
fileManifest->push_back({ "M3", fileDataID, out });
}
