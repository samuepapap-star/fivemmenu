#pragma once
#include <Includes/CustomWidgets/Custom.hpp>
#include <Bridge/Features.hpp>
#include <Bridge/ModelCatalog.hpp>
namespace BridgePages {
inline void Footer(){
 BridgeFeatures::Status();
 if(MenuBridge::Connection().Busy()&&Custom::Button("Cancelar acao",ImVec2(200,30),0))MenuBridge::Connection().Cancel();
}
inline void Render(int section){
 ImGui::SetCursorPos(ImVec2(184,136));ImGui::BeginChild("Acoes##Bridge",ImVec2(704,440),false,0);ImGui::PushID(section);
 if(section==0){
  ImGui::TextDisabled("Personagem / modelos e animais");
  static int selected=0;static char name[96]="a_c_chimp";
  const char* models[]={"a_c_chimp","a_c_mtlion","a_c_husky","a_c_cat_01","a_c_boar","a_c_cow","a_c_crow","s_m_y_cop_01","s_m_y_swat_01","mp_m_freemode_01","mp_f_freemode_01"};
  if(ImGui::Combo("Modelo predefinido",&selected,"Chimpanze\0Puma\0Cachorro\0Gato\0Javali\0Vaca\0Corvo\0Policial\0SWAT\0Freemode masculino\0Freemode feminino\0"))MenuBridge::Copy(name,sizeof(name),models[selected]);
  ImGui::InputText("Nome do modelo",name,sizeof(name));
  if(Custom::Button("Transformar personagem",ImVec2(-1,33),0))BridgeFeatures::Model(name);
  if(Custom::Button("Restaurar personagem original",ImVec2(-1,33),0))BridgeFeatures::Restore();
  ImGui::TextWrapped("A aparencia original e guardada antes da primeira troca. Saia do veiculo. A troca desativa God, NoClip e camera livre.");
 }
 if(section==2){
  ImGui::TextDisabled("Carro / gerar veiculo");static char model[96]="adder";static bool network=true,enter=true;
  static const std::vector<MenuBridge::VehicleModel> defaults={{"adder",7},{"zentorno",7},{"t20",7},{"sultan",6},{"blista",0},{"bati",8},{"faggio",8},{"bmx",13}};
  static std::vector<MenuBridge::VehicleModel> registered;static std::uint64_t cachedToken=0;static char search[96]{};
  const auto token=MenuBridge::Connection().CatalogToken();
  if(token!=cachedToken&&!(token&1u)){registered=MenuBridge::Connection().Models();cachedToken=token;}
  if(Custom::Button("Carregar lista de modelos",ImVec2(-1,30),0))BridgeFeatures::LoadVehicleCatalog();
  if(registered.empty())ImGui::TextDisabled("Predefinidos; carregue os modelos da sessao");
  else ImGui::TextDisabled("Modelos registrados: %u",static_cast<unsigned>(registered.size()));
  ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("Buscar veiculo","Digite parte do nome do modelo...",search,sizeof(search));
  const auto& models=registered.empty()?defaults:registered;
  const auto filter=MenuBridge::LowerModel(search);std::vector<std::size_t> filtered;
  for(std::size_t i=0;i<models.size();i++)if(models[i].name.find(filter)!=std::string::npos)filtered.push_back(i);
  ImGui::TextDisabled("Resultados: %u",static_cast<unsigned>(filtered.size()));
  ImGui::SetNextItemWidth(-1);
  const int resultCount=static_cast<int>(filtered.size());
  const int visibleRows=resultCount<1?1:(resultCount>8?8:resultCount);
  const float rowHeight=ImGui::GetTextLineHeight();
  const float popupHeight=(rowHeight+ImGui::GetStyle().ItemSpacing.y)*visibleRows+ImGui::GetStyle().WindowPadding.y*2.f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(0,popupHeight),ImVec2(10000.f,popupHeight));
  // Custom BeginCombo uses its third parameter as the result count, not flags.
  if(ImGui::BeginCombo("Escolher veiculo",model,resultCount)){
   ImGuiListClipper clipper;clipper.Begin(resultCount,rowHeight+ImGui::GetStyle().ItemSpacing.y);
   while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;i++){
    const auto& code=models[filtered[static_cast<std::size_t>(i)]].name;
    if(ImGui::Selectable(code.c_str(),code==model,0,ImVec2(0,rowHeight))){MenuBridge::Copy(model,sizeof(model),code);ImGui::CloseCurrentPopup();}
   }
   if(filtered.empty())ImGui::TextDisabled("Nenhum veiculo encontrado com esse nome");
   ImGui::EndCombo();
  }
  ImGui::SetNextItemWidth(360);ImGui::InputText("Nome personalizado",model,sizeof(model));
  Custom::CheckBox("Gerar em rede (solicitar sincronizacao)",&network);Custom::CheckBox("Entrar no veiculo gerado",&enter);
  if(Custom::Button("Gerar veiculo",ImVec2(-1,33),0))BridgeFeatures::Spawn(model,network,enter);
  ImGui::TextWrapped(network?"Modo em rede: solicita sincronizacao; visibilidade para outros depende do servidor.":"Modo local: o veiculo aparece somente para voce.");
 }
 Footer();ImGui::PopID();ImGui::EndChild();
}
}
