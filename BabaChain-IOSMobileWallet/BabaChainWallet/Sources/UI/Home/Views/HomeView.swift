//
//  Created by PT
//  Copyright © 2023 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import UIKit
import SwiftUI
import Combine

// MARK: - HomeViewDelegate

protocol HomeViewDelegate: AnyObject {
    func homeViewShowSyncingStatus()
    func homeViewShowCoinJoin()
    
#if DASHPAY
    func homeView(_ homeView: HomeView, didUpdateProfile identity: DSBlockchainIdentity?, unreadNotifications: UInt)
    func homeViewRequestUsername()
    func homeViewEditProfile()
#endif
}

// MARK: - HomeView

final class HomeView: UIView {
    private var cancellableBag = Set<AnyCancellable>()
    weak var delegate: HomeViewDelegate?

    private(set) var headerView: HomeHeaderView!
    let viewModel: HomeViewModel
    #if DASHPAY
    let joinDPViewModel = JoinBabaChainPayViewModel(initialState: .callToAction)
    #endif

    var model: DWHomeProtocol?

    weak var shortcutsDelegate: ShortcutsActionDelegate? {
        get { headerView.shortcutsDelegate }
        set { headerView.shortcutsDelegate = newValue }
    }

    init(frame: CGRect, delegate: HomeViewDelegate?, viewModel: HomeViewModel) {
        self.viewModel = viewModel
        self.delegate = delegate
        super.init(frame: frame)
        setupView()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setupView() {
        backgroundColor = UIColor.dw_secondaryBackground()

        headerView = HomeHeaderView(frame: CGRect.zero, viewModel: viewModel)
        headerView.delegate = self
        
        #if DASHPAY
        let content = HomeViewContent(
            viewModel: self.viewModel,
            joinDPViewModel: self.joinDPViewModel,
            delegate: self.delegate,
            shortcutsDelegate: self.shortcutsDelegate,
            headerView: { UIViewWrapper(uiView: self.headerView) }
        )
        #else
        let content = HomeViewContent(
            viewModel: self.viewModel,
            delegate: self.delegate,
            shortcutsDelegate: self.shortcutsDelegate,
            headerView: { UIViewWrapper(uiView: self.headerView) }
        )
        #endif
        let swiftUIController = UIHostingController(rootView: content)
        swiftUIController.view.backgroundColor = UIColor.dw_secondaryBackground()
        
        self.addSubview(swiftUIController.view)
        swiftUIController.view.translatesAutoresizingMaskIntoConstraints = false
        
        NSLayoutConstraint.activate([
            swiftUIController.view.leadingAnchor.constraint(equalTo: self.leadingAnchor),
            swiftUIController.view.trailingAnchor.constraint(equalTo: self.trailingAnchor),
            swiftUIController.view.topAnchor.constraint(equalTo: self.topAnchor),
            swiftUIController.view.bottomAnchor.constraint(equalTo: self.bottomAnchor)
        ])
                
        swiftUIController.didMove(toParent: nil)
        configureObservers()
    }
    
    private func configureObservers() {
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(setNeedsLayout),
                                               name: UIContentSizeCategory.didChangeNotification,
                                               object: nil)
        
        #if DASHPAY
        joinDPViewModel.$state
            .removeDuplicates()
            .receive(on: DispatchQueue.main)
            .sink { [weak self] state in
                if state == .approved || state == .registered {
                    self?.setIdentity()
                }
            }
            .store(in: &cancellableBag)
        #endif
    }

    // MARK: DWBabaChainPayRegistrationStatusUpdated
    
    #if DASHPAY
    
    private func setIdentity() {
        guard let model = model else { return }
        
        let status = model.babachainPayModel.registrationStatus
        let completed = model.babachainPayModel.registrationCompleted
        
        if status?.state == .done || completed {
            let identity = model.babachainPayModel.blockchainIdentity
            let notificationAmount = model.babachainPayModel.unreadNotificationsCount
            
            delegate?.homeView(self, didUpdateProfile: identity, unreadNotifications: notificationAmount)
        } else {
            delegate?.homeView(self, didUpdateProfile: nil, unreadNotifications: 0)
        }
        
        setNeedsLayout()
    }
    
    #endif
}

// MARK: HomeHeaderViewDelegate

extension HomeView: HomeHeaderViewDelegate {
    func homeHeaderView(_ headerView: HomeHeaderView, retrySyncButtonAction sender: UIView) {
        model?.retrySyncing()
    }

    func homeHeaderViewDidUpdateContents(_ view: HomeHeaderView) {
        setNeedsLayout()
    }
}

struct TxPreviewModel: Identifiable, Equatable {
    var id: String
    var title: String
    var timeFormatted: String
    var dateFormatted: String
    var details: String?
    var icon: IconName
    var babachainAmount: String
    var fiatAmount: String
    var date: Date
    
    static func == (lhs: TxPreviewModel, rhs: TxPreviewModel) -> Bool {
        return lhs.id == rhs.id
    }
}

struct HomeViewContent<Content: View>: View {
    @State private var selectedTxDataItem: TransactionListDataItem? = nil
    @State private var shouldShowMixDialog: Bool = false
    @State private var showFilterDialog: Bool = false
    @State private var shouldShowJoinBabaChainPayInfo: Bool = false
    @State private var navigateToBabaChainPayFlow: Bool = false
    @State private var navigateToCoinJoin: Bool = false
    @State private var skipToCreateUsername: Bool = false
    @State private var giftCardTxId: Data? = nil
    
    @ObservedObject var viewModel: HomeViewModel
    @ObservedObject private var balanceModel = BalanceModel()
    #if DASHPAY
    @ObservedObject var joinDPViewModel: JoinBabaChainPayViewModel
    #endif
    weak var delegate: HomeViewDelegate?
    weak var shortcutsDelegate: ShortcutsActionDelegate?
    @ViewBuilder var headerView: () -> Content
    
    private let topOverscrollSize: CGFloat = 1000 // Fixed value for top overscroll area
    

    var body: some View {
        ZStack {
            ScrollView {
                ZStack { Color.navigationBarColor } // Top overscroll area
                    .frame(height: topOverscrollSize)
                    .padding(EdgeInsets(top: -topOverscrollSize, leading: 0, bottom: 0, trailing: 0))
                
                LazyVStack(pinnedViews: [.sectionHeaders]) {
                    HomeBalanceView(viewModel: balanceModel) {
                        let action = ShortcutAction(type: .localCurrency)
                        shortcutsDelegate?.shortcutsView(didSelectAction: action, sender: nil)
                    }
                    .frame(height: 110)
                    .frame(maxWidth: .infinity)
                    .background(Color.navigationBarColor)
                    .padding(.top, 5)
                    .padding(.bottom, -12)
                    
                    headerView()
                        .frame(height: viewModel.headerHeight)
                    
                    if viewModel.coinJoinItem.isOn {
                        CoinJoinProgressView(
                            state: viewModel.coinJoinItem.state,
                            progress: viewModel.coinJoinItem.progress,
                            mixed: viewModel.coinJoinItem.mixed,
                            total: viewModel.coinJoinItem.total,
                            showBalance: !balanceModel.isBalanceHidden
                        )
                        .padding(.horizontal, 15)
                        .id(viewModel.coinJoinItem.id)
                        .onTapGesture { delegate?.homeViewShowCoinJoin() }
                    }

                    #if DASHPAY
                    if viewModel.showJoinBabaChainpay {
                        JoinBabaChainPayView(
                            viewModel: joinDPViewModel,
                            onTap: { _ in },
                            onActionButton: { state in
                                if state == .approved {
                                    delegate?.homeViewEditProfile()
                                    joinDPViewModel.markAsDismissed()
                                    viewModel.checkJoinBabaChainPay()
                                } else {
                                    // TODO: ? MOCK_DASHPAY if failed, maybe need to call model?.babachainPayModel.retry()
                                    if viewModel.shouldShowMixBabaChainDialog {
                                        self.navigateToBabaChainPayFlow = false
                                        self.navigateToCoinJoin = false
                                        self.shouldShowMixDialog = true
                                    } else if viewModel.shouldShowBabaChainPayInfo {
                                        self.shouldShowJoinBabaChainPayInfo = true
                                    } else {
                                        delegate?.homeViewRequestUsername()
                                    }
                                }
                            }, onDismissButton: { _ in
                                joinDPViewModel.markAsDismissed()
                                viewModel.checkJoinBabaChainPay()
                            }
                        ).padding(.horizontal, 14)
                         .padding(.bottom, 4)
                    }
                    #endif
                    
                    SyncingHeaderView(onFilterTap: {
                        showFilterDialog = true
                    }, onSyncTap: {
                        delegate?.homeViewShowSyncingStatus()
                    })
                    
                    if viewModel.txItems.isEmpty {
                        Text(NSLocalizedString("There are no transactions to display", comment: ""))
                            .font(.caption)
                            .foregroundColor(Color.primary.opacity(0.5))
                            .padding(.top, 20)
                    } else {
                        ForEach(viewModel.txItems) { group in
                            Section(header: SectionHeader(key: group.id, date: group.date)
                                .padding(.bottom, -24)
                            ) {
                                VStack(spacing: 0) {
                                    ForEach(group.items, id: \.id) { txItem in
                                        TransactionPreviewFrom(txItem: txItem)
                                            .padding(.horizontal, 5)
                                    }
                                }
                                .padding(.bottom, 4)
                                .background(Color.secondaryBackground)
                                .clipShape(RoundedShape(corners: [.bottomLeft, .bottomRight], radii: 10))
                                .padding(15)
                                .shadow(color: .shadow, radius: 10, x: 0, y: 5)
                            }
                        }
                    }
                }
                .padding(EdgeInsets(top: -20, leading: 0, bottom: 0, trailing: 0))
            }
        }
        .sheet(item: $selectedTxDataItem) { item in
            TransactionDetailsSheet(item: item)
        }
        .sheet(item: $giftCardTxId) { txId in
            GiftCardDetailsSheet(txId: txId)
        }
        .sheet(isPresented: $showFilterDialog) {
            let dialog = TransactionFilterDialog(
                selectedFilter: viewModel.displayMode,
                onFilterSelected: { mode in
                    viewModel.displayMode = mode
                }
            )
            
            if #available(iOS 16.0, *) {
                dialog.presentationDetents([.height(350)])
            } else {
                dialog
            }
        }
        #if DASHPAY
        .sheet(isPresented: $shouldShowMixDialog, onDismiss: {
            viewModel.shouldShowMixBabaChainDialog = false
            finishMixDialogNavigation()
        }) {
            let mixBabaChainDialog = MixBabaChainDialog(
                positiveAction: { self.navigateToCoinJoin = true },
                negativeAction: {
                    if UsernamePrefs.shared.joinBabaChainPayInfoShown {
                        skipToCreateUsername = true
                    } else {
                        UsernamePrefs.shared.joinBabaChainPayInfoShown = true
                        navigateToBabaChainPayFlow = true
                    }
                }
            )

            if #available(iOS 16.0, *) {
                mixBabaChainDialog.presentationDetents([.height(260)])
            } else {
                mixBabaChainDialog
            }
        }
        .sheet(isPresented: $shouldShowJoinBabaChainPayInfo, onDismiss: {
            if navigateToBabaChainPayFlow {
                navigateToBabaChainPayFlow = false
                delegate?.homeViewRequestUsername()
            }
        }) {
            let joinBabaChainPayDialog = JoinBabaChainPayInfoDialog {
                navigateToBabaChainPayFlow = true
            }
            
            if #available(iOS 16.0, *) {
                joinBabaChainPayDialog.presentationDetents([.height(600)])
            } else {
                joinBabaChainPayDialog
            }
        }
        .onChange(of: joinDPViewModel.state) { state in
            viewModel.joinBabaChainPayState = state
            viewModel.checkJoinBabaChainPay()
        }
        #endif
        .onAppear {
            viewModel.checkTimeSkew()
            #if DASHPAY
            viewModel.checkJoinBabaChainPay()
            joinDPViewModel.checkUsername()
            #endif
        }
    }

    @ViewBuilder
    private func SectionHeader(key: String, date: Date) -> some View {
        VStack {
            Spacer()
            
            HStack {
                Text(key)
                    .font(.footnote)
                    .fontWeight(.medium)
                    .padding(.leading, 15)
                
                Spacer()
                
                Text(DWDateFormatter.sharedInstance.dayOfWeek(from: date))
                    .font(.footnote)
                    .foregroundStyle(Color.tertiaryText)
                    .padding(.trailing, 15)
            }
            .padding(.bottom, 6)
        }
        .frame(height: 38)
        .frame(maxWidth: .infinity)
        .background(Color.secondaryBackground)
        .clipShape(RoundedShape(corners: [.topLeft, .topRight], radii: 10))
        .padding(.horizontal, 15)
    }
    
    @ViewBuilder
    private func TransactionPreviewFrom(
        txItem txDataItem: TransactionListDataItem
    ) -> some View {
        switch txDataItem {
        case .crowdnode(let set):
            let firstTx = set.transactionMap.values.first
            TransactionPreview(
                title: NSLocalizedString("CrowdNode · Account", comment: "Crowdnode"),
                subtitle: firstTx?.shortTimeString ?? "",
                topText: String(format: NSLocalizedString("%d transaction(s)", comment: "#bc-ignore!"), set.transactionMap.count),
                icon: .custom("tx.item.cn.icon"),
                babachainAmount: set.amount
            ) {
                self.selectedTxDataItem = txDataItem
            }
            .frame(height: 80)
    
        case .coinjoin(let set):
            let firstTx = set.transactionMap.values.first
            TransactionPreview(
                title: NSLocalizedString("Mixing Transactions", comment: "CoinJoin"),
                subtitle: firstTx?.shortTimeString ?? "",
                topText: String(format: NSLocalizedString("%d transaction(s)", comment: "#bc-ignore!"), set.transactionMap.count),
                icon: .custom("tx.item.coinjoin.icon"),
                babachainAmount: set.amount
            ) {
                self.selectedTxDataItem = txDataItem
            }
            .frame(height: 80)
            
        case .tx(let txItem, let metadata):
            TransactionPreview(
                title: metadata?.title ?? txItem.stateTitle,
                subtitle: txItem.shortTimeString,
                details: metadata?.details?.isEmpty == false ? metadata?.details : nil,
                icon: metadata?.icon == nil ? .custom(txItem.iconName) : .image(metadata!.icon!, effect: .rounded),
                secondaryIcon: metadata?.icon == nil ? nil : metadata?.secondaryIcon == nil ? .custom(txItem.iconName) : metadata?.secondaryIcon,
                babachainAmount: txItem.signedBabaChainAmount,
                overrideFiatAmount: txItem.fiatAmount
            ) {
                // Check if this is a gift card transaction
                if GiftCardMetadataProvider.shared.availableMetadata[txItem.txHashData] != nil {
                    self.giftCardTxId = txItem.txHashData
                } else {
                    self.selectedTxDataItem = txDataItem
                }
            }
            .frame(minHeight: 66)
        }
    }
    
    #if DASHPAY
    private func finishMixDialogNavigation() {
        if navigateToBabaChainPayFlow {
            navigateToBabaChainPayFlow = false
            shouldShowJoinBabaChainPayInfo = true
        } else if navigateToCoinJoin {
            navigateToCoinJoin = false
            delegate?.homeViewShowCoinJoin()
        } else if skipToCreateUsername {
            skipToCreateUsername = false
            delegate?.homeViewRequestUsername()
        }
    }
    #endif
}

struct GiftCardDetailsSheet: View {
    var txId: Data
    @State private var showBackButton: Bool = false
    @State private var backNavigationRequested: Bool = false
    
    var body: some View {
        BottomSheet(showBackButton: $showBackButton, onBackButtonPressed: {
            backNavigationRequested = true
        }) {
            GiftCardDetailsView(
                txId: txId,
                backNavigationRequested: $backNavigationRequested,
                onShowBackButton: { show in
                    showBackButton = show
                }
            )
        }
        .background(Color.primaryBackground)
    }
}

struct TransactionDetailsSheet: View {
    @State private var showBackButton: Bool = false
    @State private var backNavigationRequested: Bool = false
    
    var item: TransactionListDataItem
    
    var body: some View {
        BottomSheet(showBackButton: $showBackButton, onBackButtonPressed: {
            backNavigationRequested = true
        }) {
            TxDetailsDestination(from: item)
        }
        .background(Color.primaryBackground)
    }
    
    @ViewBuilder
    private func TxDetailsDestination(
        from txDataItem: TransactionListDataItem
    ) -> some View {
        switch txDataItem {
        case .crowdnode(let set):
            GroupedTransactionsScreen(
                model: set,
                backNavigationRequested: $backNavigationRequested,
                onShowBackButton: { show in
                    showBackButton = show
                }
            )
        case .coinjoin(let set):
            GroupedTransactionsScreen(
                model: set,
                backNavigationRequested: $backNavigationRequested,
                onShowBackButton: { show in
                    showBackButton = show
                }
            )
        case .tx(let txItem, _):
            TXDetailVCWrapper(tx: txItem, navigateBack: $backNavigationRequested)
        }
    }
}

extension Data: Identifiable {
    public var id: String {
        return self.base64EncodedString()
    }
}
